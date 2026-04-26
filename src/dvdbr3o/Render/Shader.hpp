#pragma once

#include "dvdbr3o/Render/Context.hpp"

#include <slang.h>
#include <webgpu/webgpu_cpp.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <string_view>

namespace dvdbr3o::Render {
inline auto layout_of_parsed(std::string_view layout_json, Context& ctx = Context::global())
	-> wgpu::BindGroupLayout {
	auto parsed = nlohmann::json::parse(layout_json);
	spdlog::info("parsed: {}", nlohmann::to_string(parsed));
	return {};
}

}  // namespace dvdbr3o::Render