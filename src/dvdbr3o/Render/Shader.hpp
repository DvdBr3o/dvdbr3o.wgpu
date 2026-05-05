#pragma once

#include "dvdbr3o/Render/Context.hpp"

#include <slang.h>
#include <slang-cpp-types.h>
#include <slang-com-ptr.h>
#include <webgpu/webgpu_cpp.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <string_view>

namespace dvdbr3o::Render {
struct LayoutJson : public std::string_view {
	using std::string_view::string_view;

	constexpr LayoutJson(std::string_view s) : std::string_view(s) {}
};

struct WgslSource : public std::string_view {
	using std::string_view::string_view;

	constexpr WgslSource(std::string_view s) : std::string_view(s) {}
};

struct SlangSource : public std::string_view {
	using std::string_view::string_view;

	constexpr SlangSource(std::string_view s) : std::string_view(s) {}
};

inline auto layout_of_parsed(LayoutJson layout_json, Context& ctx = Context::global())
	-> wgpu::BindGroupLayout {
	auto parsed = nlohmann::json::parse(layout_json);
	spdlog::info("parsed: {}", nlohmann::to_string(parsed));
	return {};
}

inline auto shader_variant(SlangSource src) {
	//
}

class SlangManager {
public:
	SlangManager() {
		const SlangGlobalSessionDesc desc = {};
		slang::createGlobalSession(&desc, _global_session.writeRef());
	}

	constexpr operator Slang::ComPtr<slang::IGlobalSession>&() { return _global_session; }

	constexpr operator const Slang::ComPtr<slang::IGlobalSession>&() const {
		return _global_session;
	}

	inline static decltype(auto) create_session(const slang::SessionDesc& desc) {
		return utils()._create_session(desc);
	}

	[[nodiscard]] auto _create_session(const slang::SessionDesc& desc) const
		-> Slang::ComPtr<slang::ISession> {
		Slang::ComPtr<slang::ISession> session;
		_global_session->createSession(desc, session.writeRef());
		return session;
	}

public:
	inline static auto utils() -> SlangManager& {
		static SlangManager manager;
		return manager;
	}

	inline static auto& global() {
		static SlangManager manager;
		return manager._global_session;
	}

private:
	Slang::ComPtr<slang::IGlobalSession> _global_session;
};

inline auto& slang_manager() {
	return SlangManager::global();
}

inline auto& slang_manager_utils() {
	return SlangManager::utils();
}

inline auto _foo() {
	auto session = SlangManager::create_session({});
}

}  // namespace dvdbr3o::Render