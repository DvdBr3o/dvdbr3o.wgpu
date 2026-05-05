#pragma once

#include "dvdbr3o/Render/Context.hpp"

#include <webgpu/webgpu_cpp.h>
#include <nlohmann/json.hpp>

#include <span>

namespace dvdbr3o::Render {
inline auto bindgroup_from(
	const wgpu::BindGroupLayout& layout, std::span<const wgpu::BindGroupEntry> entries,
	const Context& ctx = Context::global()
) -> wgpu::BindGroup {
	const wgpu::BindGroupDescriptor desc = {
		.layout		= layout,
		.entryCount = entries.size(),
		.entries	= entries.data(),
	};
	return ctx.device.CreateBindGroup(&desc);
}

inline auto bindgroup_from(
	const Context& ctx, const wgpu::BindGroupLayout& layout,
	std::span<const wgpu::BindGroupEntry> entries
) -> wgpu::BindGroup {
	return bindgroup_from(layout, entries, ctx);
}

struct LayoutJsonParseError : UnlikelyExcption {
	using UnlikelyExcption::UnlikelyExcption;
};

inline auto view_dimension_from(std::string_view s) -> wgpu::TextureViewDimension {
	if (s == "e1D")
		return wgpu::TextureViewDimension::e1D;
	else if (s == "e2D")
		return wgpu::TextureViewDimension::e2D;
	else if (s == "e3D")
		return wgpu::TextureViewDimension::e3D;
	else [[unlikely]]
		throw LayoutJsonParseError("unexpected view dimension `{}`!", s);
}

inline auto texture_format_from(std::string_view format) -> wgpu::TextureFormat {
	if (format == "R32Float")
		return wgpu::TextureFormat::R32Float;
	if (format == "R16Float")
		return wgpu::TextureFormat::R16Float;
	if (format == "R32Uint")
		return wgpu::TextureFormat::R32Uint;
	if (format == "RG32Float")
		return wgpu::TextureFormat::RG32Float;
	if (format == "RGBA32Float")
		return wgpu::TextureFormat::RGBA32Float;
	if (format == "RGBA8Unorm")
		return wgpu::TextureFormat::RGBA8Unorm;

	[[unlikely]] { throw LayoutJsonParseError("unknown texture format `{}`!", format); }
}

inline auto texture_format_from(const nlohmann::json& binding) -> wgpu::TextureFormat {
	const auto format_it = binding.find("format");
	if (format_it == binding.end() || !format_it->is_string())
		return wgpu::TextureFormat::RGBA8Unorm;
	return texture_format_from(format_it->get<std::string_view>());
}

inline auto bindgroup_layout_from_parameter(const Context& ctx, const nlohmann::json& param)
	-> wgpu::BindGroupLayout {
	//
	std::vector<wgpu::BindGroupLayoutEntry> entries;
	entries.reserve(param["bindings_count"]);

	for (auto&& binding : param["bindings"]) {
		std::string_view kind = binding["kind"];
		if (kind == "uniform")
			entries.emplace_back(wgpu::BindGroupLayoutEntry {
						.binding = (uint32_t)binding["binding"],
						.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute,
						.buffer = {
							.type = wgpu::BufferBindingType::Uniform,
							.minBindingSize = binding["size"],
						},
					});
		else if (kind == "texture")
			entries.emplace_back(wgpu::BindGroupLayoutEntry {
						.binding = (uint32_t)binding["binding"],
						.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute,
						.texture = {
							.sampleType = wgpu::TextureSampleType::Float,
							.viewDimension = view_dimension_from(binding["viewDimension"]),
							// .multisampled = true,
						},
					});
		else if (kind == "storageTexture") {
			const std::string_view	   access_entry = binding["access"];
			wgpu::StorageTextureAccess access;
			if (access_entry == "readWrite") [[likely]]
				access = wgpu::StorageTextureAccess::ReadWrite;
			else if (access_entry == "readOnly")
				access = wgpu::StorageTextureAccess::ReadOnly;
			else if (access_entry == "writeOnly")
				access = wgpu::StorageTextureAccess::WriteOnly;
			else if (access_entry == "bindingNotUsed")
				access = wgpu::StorageTextureAccess::BindingNotUsed;
			else [[unlikely]]
				throw LayoutJsonParseError("unkown storage texture access `{}`!", access_entry);
			entries.emplace_back(wgpu::BindGroupLayoutEntry {
				.binding = (uint32_t)binding["binding"],
				.visibility = wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute,
				.storageTexture = {
					.access = access,
					.format = texture_format_from(binding),
					.viewDimension = view_dimension_from(binding["viewDimension"]),
				},
			});
		} else if (kind == "sampler")
			entries.emplace_back(
						wgpu::BindGroupLayoutEntry {
							.binding	= (uint32_t)binding["binding"],
							.visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Compute,
							.sampler	= {
								.type = wgpu::SamplerBindingType::Filtering,
							},
						}
					);
		else [[unlikely]]
			throw LayoutJsonParseError("unexpected binding kind `{}`!", kind);
	}

	const wgpu::BindGroupLayoutDescriptor desc {
		.entryCount = entries.size(),
		.entries	= entries.data(),
	};
	return ctx.device.CreateBindGroupLayout(&desc);
}

inline auto bindgroup_layout_from_parameter(const nlohmann::json& param) -> wgpu::BindGroupLayout {
	return bindgroup_layout_from_parameter(Context::global(), param);
}

inline auto bindgroup_layout_from_string(
	const Context& ctx, std::string_view name, std::string_view layout_json
) {
	return bindgroup_layout_from_parameter(
		ctx,
		nlohmann::json::parse(layout_json)["parameters"][name]
	);
}

inline auto bindgroup_layout_from_string(std::string_view name, std::string_view layout_json) {
	return bindgroup_layout_from_string(Context::global(), name, layout_json);
}

inline auto bindgroup_layouts(const Context& ctx, const nlohmann::json& json)
	-> std::vector<wgpu::BindGroupLayout> {
	auto parameters = json["all"];

	//
	std::vector<wgpu::BindGroupLayout> layouts;
	layouts.reserve(parameters.size());

	for (auto&& parameter : parameters)
		layouts.emplace_back(bindgroup_layout_from_parameter(ctx, parameter));
	return layouts;
}

inline auto bindgroup_layouts(const nlohmann::json& json) -> std::vector<wgpu::BindGroupLayout> {
	return bindgroup_layouts(Context::global(), json);
}

inline auto bindgroup_layouts_from_string(const Context& ctx, std::string_view json)
	-> std::vector<wgpu::BindGroupLayout> {
	return bindgroup_layouts(ctx, nlohmann::json::parse(json));
}

inline auto bindgroup_layouts_from_string(std::string_view json)
	-> std::vector<wgpu::BindGroupLayout> {
	return bindgroup_layouts_from_string(Context::global(), json);
}

}  // namespace dvdbr3o::Render
