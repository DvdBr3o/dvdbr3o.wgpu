#pragma once

#include "dvdbr3o/Render/Context.hpp"
#include "dvdbr3o/Utils/Misc.hpp"

#include <webgpu/webgpu_cpp.h>
#include <slang.h>
#include <stdexec/execution.hpp>

#include <filesystem>

namespace dvdbr3o::Render {
inline auto wgsl_module_from_source(std::string_view source, Context& ctx = Context::global()) {
	const wgpu::ShaderSourceWGSL	   wgsl { { .code = source } };
	const wgpu::ShaderModuleDescriptor shader_module_desc = { .nextInChain = &wgsl };
	return ctx.device.CreateShaderModule(&shader_module_desc);
}

inline auto async_wgsl_module_from_path(
	const std::filesystem::path& path, auto executor, Context& ctx = Context::global()
) {
	using stdexec::then;
	return									  //
		async_read_text_from(path, executor)  //
		| then([&](auto&& source) { return wgsl_module_from_source(source, ctx); });
}

inline auto wgsl_module_from_path(
	const std::filesystem::path& path, Context& ctx = Context::global()
) {
	return wgsl_module_from_source(read_text_from(path).value(), ctx);
}

}  // namespace dvdbr3o::Render
