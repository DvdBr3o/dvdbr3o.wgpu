#pragma once

#include "dvdbr3o/Render/Context.hpp"

#include <webgpu/webgpu_cpp.h>

namespace dvdbr3o::Render {
template<typename T, wgpu::BufferUsage usage>
inline auto array_buffer(const Context& ctx, std::span<const T> data) -> wgpu::Buffer {
	const wgpu::BufferDescriptor desc {
		.usage			  = usage,
		.size			  = sizeof(T) * data.size(),
		.mappedAtCreation = false,
	};
	wgpu::Buffer buffer = ctx.device.CreateBuffer(&desc);

	ctx.queue.WriteBuffer(buffer, 0, data.data(), sizeof(T) * data.size());

	return buffer;
}

template<typename T, wgpu::BufferUsage usage>
inline auto array_buffer(std::span<const T> data) -> wgpu::Buffer {
	return array_buffer(Context::global(), data);
}

template<typename VerticeT>
inline auto array_vertex_buffer(const Context& ctx, std::span<const VerticeT> data)
	-> wgpu::Buffer {
	return array_buffer<VerticeT, wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst>(
		ctx,
		data
	);
}

template<typename VerticeT>
inline auto array_vertex_buffer(std::span<const VerticeT> data) -> wgpu::Buffer {
	return array_vertex_buffer<VerticeT>(Context::global(), data);
}

template<typename T>
inline auto array_index_buffer(const Context& ctx, std::span<const T> data) -> wgpu::Buffer {
	return array_buffer<T, wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst>(ctx, data);
}

template<typename T>
inline auto array_index_buffer(std::span<const T> data) -> wgpu::Buffer {
	return array_index_buffer<T>(Context::global(), data);
}

template<wgpu::BufferUsage usage>
inline auto create_buffer(size_t size, const Context& ctx = Context::global()) -> wgpu::Buffer {
	const wgpu::BufferDescriptor desc = {
		.usage			  = usage,
		.size			  = size,
		.mappedAtCreation = false,
	};
	return ctx.device.CreateBuffer(&desc);
}
}  // namespace dvdbr3o::Render