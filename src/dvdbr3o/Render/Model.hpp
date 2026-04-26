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
#include <spdlog/spdlog.h>
#include <webgpu/webgpu_cpp.h>

#include <span>
#include <optional>

namespace dvdbr3o::Render {
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

	auto asset = ModelLoader::global().loadGltf(fastgltf::GltfDataBuffer::FromSpan(gltf).get(), "");
	if (!asset)
		throw GltfParseError("model fail: {}", (uint64_t)asset.error());

	meshes.reserve(asset->meshes.size());

	for (const auto& mesh : asset->meshes) {
		Mesh out_mesh;
		out_mesh.primitives.reserve(mesh.primitives.size());
		for (const auto& prim : mesh.primitives) {
			MeshPrimitive out_primitive;

			if (prim.materialIndex.has_value()) {
				const auto& material = asset->materials[prim.materialIndex.value()];
				const auto& texture =
					asset->textures[material.pbrData.baseColorTexture->textureIndex];
				if (texture.imageIndex.has_value()) {
					auto& image				 = asset->images[texture.imageIndex.value()];
					out_primitive.tex_albedo = texture_from_image(asset.get(), image);
				} else [[unlikely]] {
					throw GltfParseError("texture abledo does not have imageIndex!");
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
							vertices[idx].pos[0] = v.x();
							vertices[idx].pos[1] = v.y();
							vertices[idx].pos[2] = v.z();
						}
					);
				} else [[unlikely]] {
					throw GltfParseError("wtf this gltf has no attribute `POSITION`?");
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

				out_primitive.buf_vertex =
					Render::array_vertex_buffer<Render::Vertice>(std::move(vertices));
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
