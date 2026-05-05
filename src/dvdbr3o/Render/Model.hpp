#pragma once

#include "dvdbr3o/Render/Utils.hpp"
#include "dvdbr3o/Render/Bindgroup.hpp"
#include "dvdbr3o/Render/Sampler.hpp"
#include "dvdbr3o/Render/Vertice.hpp"
#include "dvdbr3o/Render/Buffer.hpp"
#include "dvdbr3o/Shaders/Embed.hpp"
#include "dvdbr3o/Utils/Misc.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/math.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <spdlog/spdlog.h>
#include <webgpu/webgpu_cpp.h>

#include <span>
#include <optional>
#include <array>
#include <string_view>
#include <cstdint>

namespace dvdbr3o::Render {
namespace MeshPipelineLabel {
inline constexpr std::string_view mtoon = "mtoon";
inline constexpr std::string_view ascii = "ascii";
}  // namespace MeshPipelineLabel

inline auto pipeline_label_from(std::string_view pipeline) -> std::string_view {
	if (pipeline == "ascii")
		return MeshPipelineLabel::ascii;
	return MeshPipelineLabel::mtoon;
}

struct MeshPrimitive {
	wgpu::Buffer			 buf_vertex;
	wgpu::VertexBufferLayout buf_vertex_layout = vertex_layout<Vertice>();
	wgpu::Buffer			 buf_index;
	size_t					 buf_index_count  = 0;
	wgpu::IndexFormat		 buf_index_format = wgpu::IndexFormat::Uint32;

	wgpu::Texture			 tex_albedo;
	wgpu::Texture			 tex_normal;

	wgpu::BindGroup			 bg_pbr;

	//
	void render(wgpu::RenderPassEncoder& pass) const {
		pass.SetBindGroup(2, bg_pbr);
		pass.SetVertexBuffer(0, buf_vertex);
		pass.SetIndexBuffer(buf_index, buf_index_format);
		pass.DrawIndexed(buf_index_count);
	}
};

struct Mesh {
	std::string_view			 pipeline = MeshPipelineLabel::mtoon;
	std::vector<MeshPrimitive> primitives;

	using Primitive = MeshPrimitive;

	void render(wgpu::RenderPassEncoder& pass) const {
		for (const auto& primitive : primitives) primitive.render(pass);
	}
};

class ModelLoader {
public:
	inline static auto& global() {
		thread_local fastgltf::Parser parser;
		return parser;
	}
};

struct GltfParseError : UnlikelyExcption {
	using UnlikelyExcption::UnlikelyExcption;
};

inline auto gltf_json_from(std::span<std::byte> gltf) -> nlohmann::json {
	const auto bytes = reinterpret_cast<const std::uint8_t*>(gltf.data());
	const auto size  = gltf.size();
	if (size < 4)
		throw GltfParseError("gltf blob too small");

	const auto read_u32 = [&](std::size_t offset) -> std::uint32_t {
		if (offset + sizeof(std::uint32_t) > size)
			throw GltfParseError("unexpected eof while reading gltf header");
		std::uint32_t value = 0;
		std::memcpy(&value, bytes + offset, sizeof(value));
		return value;
	};

	constexpr std::uint32_t glb_magic = 0x46546C67;	 // "glTF"
	if (read_u32(0) == glb_magic) {
		if (size < 20)
			throw GltfParseError("glb blob too small");
		const auto json_chunk_length = read_u32(12);
		const auto json_chunk_type	  = read_u32(16);
		constexpr std::uint32_t json_chunk_magic = 0x4E4F534A;	// "JSON"
		if (json_chunk_type != json_chunk_magic)
			throw GltfParseError("glb does not start with a json chunk");
		if (20 + json_chunk_length > size)
			throw GltfParseError("glb json chunk truncated");
		return nlohmann::json::parse(
			std::string_view {
				reinterpret_cast<const char*>(bytes + 20),
				json_chunk_length,
			}
		);
	}

	return nlohmann::json::parse(std::string_view {
		reinterpret_cast<const char*>(bytes),
		size,
	});
}

inline auto mesh_pipeline_kinds_from(
	std::span<std::byte> gltf, std::size_t mesh_count
) -> std::vector<std::string_view> {
	std::vector<std::string_view> kinds(mesh_count, MeshPipelineLabel::mtoon);
	const auto json = gltf_json_from(gltf);
	const auto nodes_it = json.find("nodes");
	if (nodes_it == json.end() || !nodes_it->is_array())
		return kinds;

	for (const auto& node : *nodes_it) {
		if (!node.is_object())
			continue;

		const auto mesh_it = node.find("mesh");
		if (mesh_it == node.end() || !mesh_it->is_number_unsigned())
			continue;

		const auto extras_it = node.find("extras");
		if (extras_it == node.end() || !extras_it->is_object())
			continue;

		const auto pipeline_it = extras_it->find("pipeline");
		if (pipeline_it == extras_it->end() || !pipeline_it->is_string())
			continue;

		const auto mesh_index = mesh_it->get<std::size_t>();
		if (mesh_index >= kinds.size())
			continue;
		kinds[mesh_index] = pipeline_label_from(pipeline_it->get<std::string_view>());
	}
	return kinds;
}

inline auto json_array_vec3_or(const nlohmann::json* value, const glm::vec3& fallback) -> glm::vec3 {
	if (value == nullptr || !value->is_array() || value->size() != 3)
		return fallback;
	return glm::vec3(
		(*value)[0].get<float>(),
		(*value)[1].get<float>(),
		(*value)[2].get<float>()
	);
}

inline auto json_array_quat_or(const nlohmann::json* value, const glm::quat& fallback) -> glm::quat {
	if (value == nullptr || !value->is_array() || value->size() != 4)
		return fallback;
	return glm::quat(
		(*value)[3].get<float>(),
		(*value)[0].get<float>(),
		(*value)[1].get<float>(),
		(*value)[2].get<float>()
	);
}

inline auto json_array_mat4_or_identity(const nlohmann::json* value) -> glm::mat4 {
	if (value == nullptr || !value->is_array() || value->size() != 16)
		return glm::mat4(1.0f);

	glm::mat4 out(1.0f);
	for (int col = 0; col < 4; ++col)
		for (int row = 0; row < 4; ++row)
			out[col][row] = (*value)[col * 4 + row].get<float>();
	return out;
}

inline auto node_local_transform_from(const nlohmann::json& node) -> glm::mat4 {
	if (!node.is_object())
		return glm::mat4(1.0f);

	const auto matrix_it = node.find("matrix");
	if (matrix_it != node.end())
		return json_array_mat4_or_identity(&*matrix_it);

	const auto translation_it = node.find("translation");
	const auto rotation_it = node.find("rotation");
	const auto scale_it = node.find("scale");
	const auto translation =
		json_array_vec3_or(translation_it != node.end() ? &*translation_it : nullptr, glm::vec3(0.0f));
	const auto rotation = json_array_quat_or(
		rotation_it != node.end() ? &*rotation_it : nullptr,
		glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
	);
	const auto scale =
		json_array_vec3_or(scale_it != node.end() ? &*scale_it : nullptr, glm::vec3(1.0f));

	return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation)
		* glm::scale(glm::mat4(1.0f), scale);
}

inline auto mesh_node_transforms_from(
	std::span<std::byte> gltf, std::size_t mesh_count
) -> std::vector<glm::mat4> {
	std::vector<glm::mat4> transforms(mesh_count, glm::mat4(1.0f));
	const auto json = gltf_json_from(gltf);
	const auto nodes_it = json.find("nodes");
	if (nodes_it == json.end() || !nodes_it->is_array())
		return transforms;

	for (const auto& node : *nodes_it) {
		if (!node.is_object())
			continue;
		const auto mesh_it = node.find("mesh");
		if (mesh_it == node.end() || !mesh_it->is_number_unsigned())
			continue;
		const auto mesh_index = mesh_it->get<std::size_t>();
		if (mesh_index >= transforms.size())
			continue;
		transforms[mesh_index] = node_local_transform_from(node);
	}
	return transforms;
}

inline auto model_from(
	std::span<std::byte> gltf, const wgpu::BindGroupLayout& pbr_layout,
	const Context& ctx = Context::global()
) -> std::vector<Mesh>;

inline auto model_from(std::span<std::byte> gltf, const Context& ctx = Context::global())
	-> std::vector<Mesh> {
	const auto pbr_layout = bindgroup_layout_from_string(ctx, "pbr", Shaders::mtoon.layout_json);
	return model_from(gltf, pbr_layout, ctx);
}

inline auto model_from(
	std::span<std::byte> gltf, const wgpu::BindGroupLayout& pbr_layout, const Context& ctx
) -> std::vector<Mesh> {
	std::vector<Mesh> meshes;
	const auto		  fallback_albedo = [&]() {
		   std::array<char, 4> white_pixel {
			   static_cast<char>(0xFF),
			   static_cast<char>(0xFF),
			   static_cast<char>(0xFF),
			   static_cast<char>(0xFF),
		   };
		   return texture_from_image(
			   Render::ImageInfo {
					   .data   = std::span<char> { white_pixel.data(), white_pixel.size() },
					   .width  = 1,
					   .height = 1,
		   },
			   ctx
		   );
	}();

	auto asset = ModelLoader::global().loadGltf(fastgltf::GltfDataBuffer::FromSpan(gltf).get(), "");
	if (!asset)
		throw GltfParseError("model fail: {}", (uint64_t)asset.error());
	const auto mesh_pipeline_kinds = mesh_pipeline_kinds_from(gltf, asset->meshes.size());
	const auto mesh_node_transforms = mesh_node_transforms_from(gltf, asset->meshes.size());

	meshes.reserve(asset->meshes.size());

	for (std::size_t mesh_index = 0; mesh_index < asset->meshes.size(); ++mesh_index) {
		const auto& mesh = asset->meshes[mesh_index];
		Mesh out_mesh;
		out_mesh.pipeline = mesh_pipeline_kinds[mesh_index];
		const auto node_transform = mesh_node_transforms[mesh_index];
		const auto normal_transform =
			glm::transpose(glm::inverse(glm::mat3(node_transform)));
		out_mesh.primitives.reserve(mesh.primitives.size());
		for (const auto& prim : mesh.primitives) {
			MeshPrimitive out_primitive;
			out_primitive.tex_albedo = fallback_albedo;

			if (prim.materialIndex.has_value()) {
				const auto& material = asset->materials[prim.materialIndex.value()];
				if (material.pbrData.baseColorTexture.has_value()) {
					const auto& texture =
						asset->textures[material.pbrData.baseColorTexture->textureIndex];
					if (texture.imageIndex.has_value()) {
						auto& image				 = asset->images[texture.imageIndex.value()];
						out_primitive.tex_albedo = texture_from_image(asset.get(), image);
					} else [[unlikely]] {
						throw GltfParseError("texture abledo does not have imageIndex!");
					}
				}

				if (material.normalTexture.has_value()) {
					auto  texIndex = material.normalTexture->textureIndex;
					auto& texture  = asset->textures[texIndex];
					if (texture.imageIndex.has_value()) {
						auto& image				 = asset->images[texture.imageIndex.value()];
						out_primitive.tex_normal = texture_from_image(asset.get(), image);
					} else [[unlikely]] {
						throw GltfParseError("texture normal does not have imageIndex!");
					}
				}
			}

			{	// pbr bg
				// clang-format off
				out_primitive.bg_pbr = bindgroup_from(  
					pbr_layout,
					std::array { 
						wgpu::BindGroupEntry {
							.binding = 0,
							.textureView = out_primitive.tex_albedo.CreateView(),
						},
						wgpu::BindGroupEntry {
							.binding = 1,
							.sampler = isotropic_sampler(wgpu::AddressMode::Repeat, wgpu::FilterMode::Nearest), // TODO: use sampler specified in gltf json
						},
					}
				 );
				// clang-format on
			}

			{  // Vertices
				std::vector<Vertice> vertices;

				if (auto it = prim.findAttribute("POSITION"); it != prim.attributes.end()) {
					auto& accessor = asset->accessors[it->accessorIndex];
					vertices.resize(accessor.count);

					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
						asset.get(),
						asset->accessors[it->accessorIndex],
						[&](fastgltf::math::fvec3 v, size_t idx) {
							const auto transformed =
								node_transform * glm::vec4(v.x(), v.y(), v.z(), 1.0f);
							vertices[idx].pos[0] = transformed.x;
							vertices[idx].pos[1] = transformed.y;
							vertices[idx].pos[2] = transformed.z;
						}
					);
				} else [[unlikely]] {
					throw GltfParseError("wtf this gltf has no attribute `POSITION`?");
				}

				if (auto it = prim.findAttribute("NORMAL"); it != prim.attributes.end()) {
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
						asset.get(),
						asset->accessors[it->accessorIndex],
						[&](fastgltf::math::fvec3 v, size_t idx) {
							const auto transformed =
								glm::normalize(normal_transform * glm::vec3(v.x(), v.y(), v.z()));
							vertices[idx].normal[0] = transformed.x;
							vertices[idx].normal[1] = transformed.y;
							vertices[idx].normal[2] = transformed.z;
						}
					);
				}

				if (auto it = prim.findAttribute("TEXCOORD_0"); it != prim.attributes.end()) {
					fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
						asset.get(),
						asset->accessors[it->accessorIndex],
						[&](fastgltf::math::fvec2 v, size_t idx) {
							vertices[idx].uv[0] = v.x();
							vertices[idx].uv[1] = v.y();
						}
					);
				} else [[unlikely]] {
					throw GltfParseError("wtf this gltf has no attribute `TEXCOORD_0`?");
				}

				out_primitive.buf_vertex = array_vertex_buffer<Vertice>(std::move(vertices));
			}

			{  // Indices
				auto&					 index_accessor = asset->accessors[*prim.indicesAccessor];
				std::vector<std::size_t> indices;
				if (prim.indicesAccessor.has_value()) {
					switch (index_accessor.componentType) {
						case fastgltf::ComponentType::UnsignedByte:
							throw GltfParseError("wgpu does not support uint8_t index yet!");
							break;
						case fastgltf::ComponentType::UnsignedShort:
							indices.resize(index_accessor.count);
							fastgltf::copyFromAccessor<std::uint16_t>(
								asset.get(),
								index_accessor,
								indices.data()
							);
							out_primitive.buf_index_format = wgpu::IndexFormat::Uint16;
							break;
						case fastgltf::ComponentType::UnsignedInt:
							indices.resize(index_accessor.count);
							fastgltf::copyFromAccessor<std::uint32_t>(
								asset.get(),
								index_accessor,
								indices.data()
							);
							out_primitive.buf_index_format = wgpu::IndexFormat::Uint32;
							break;
						default: throw GltfParseError("unexpected index type!"); break;
					}
					out_primitive.buf_index		  = array_index_buffer(to_span(indices));
					out_primitive.buf_index_count = indices.size();
				} else [[unlikely]] {
					throw GltfParseError("wtf this gltf has no indice accessor?");
				}
			}

			out_mesh.primitives.emplace_back(std::move(out_primitive));
		}
		meshes.emplace_back(std::move(out_mesh));
	}
	return meshes;
}

}  // namespace dvdbr3o::Render
