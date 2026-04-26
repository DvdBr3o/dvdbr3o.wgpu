#pragma once

#include "dvdbr3o/Render/Context.hpp"

#include <webgpu/webgpu_cpp.h>

namespace dvdbr3o::Render {
inline auto isotropic_sampler(
	const Context& ctx, wgpu::AddressMode address_mode, wgpu::FilterMode filter
) -> wgpu::Sampler {
	const wgpu::SamplerDescriptor desc {
		.addressModeU = address_mode,
		.addressModeV = address_mode,
		.addressModeW = address_mode,
		.magFilter	  = filter,
		.minFilter	  = filter,
	};
	return ctx.device.CreateSampler(&desc);
}

inline auto isotropic_sampler(wgpu::AddressMode address_mode, wgpu::FilterMode filter)
	-> wgpu::Sampler {
	return isotropic_sampler(Context::global(), address_mode, filter);
}
}  // namespace dvdbr3o::Render