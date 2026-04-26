#pragma once

#include "dvdbr3o/Utils/Misc.hpp"

#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>

#include <array>

namespace dvdbr3o::Render {
struct Vertice {
	glm::vec3					 pos;
	glm::vec3					 normal;
	glm::vec2					 uv;
	size_t						 tex_id;

	inline static constexpr auto vertex_attribute() noexcept {
		return std::to_array<wgpu::VertexAttribute>({
			{
				.format			= wgpu::VertexFormat::Float32x3,
				.offset			= offsetof(Vertice,	pos),
				.shaderLocation = 0,
			 },
			{
				.format			= wgpu::VertexFormat::Float32x3,
				.offset			= offsetof(Vertice, normal),
				.shaderLocation = 1,
			 },
			{
				.format			= wgpu::VertexFormat::Float32x2,
				.offset			= offsetof(Vertice,		uv),
				.shaderLocation = 2,
			 },
			{
				.format			= wgpu::VertexFormat::Uint32,
				.offset			= offsetof(Vertice, tex_id),
				.shaderLocation = 3,
			 },
		});
	}
};

template<typename T>
	requires requires {
		{ T::vertex_attribute() } -> FatPointerAlike;
	}
inline constexpr auto vertex_attribute = T::vertex_attribute();

template<typename T>
	requires requires {
		{ T::vertex_attribute() } -> FatPointerAlike;
	} && ConstEvaluated<T::vertex_attribute()>
inline constexpr auto vertex_layout() -> wgpu::VertexBufferLayout {
	return {
		.arrayStride	= sizeof(T),
		.attributeCount = vertex_attribute<T>.size(),
		.attributes		= vertex_attribute<T>.data(),
	};
}

}  // namespace dvdbr3o::Render