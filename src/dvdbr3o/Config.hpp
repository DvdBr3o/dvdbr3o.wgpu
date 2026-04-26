#pragma once

namespace dvdbr3o {
enum class Platform {
	Unknown = 0,
	Windows,
	Macos,
	Linux,
	Android,
	Ios,
	Wasm,
};

enum class Mode {
	Debug,
	Release,
	Profile,
};

inline constexpr Platform platform =
#if defined DVDBR3O_WGPU_PLAT_WINDOWS
	Platform::Windows
#elif defined DVDBR3O_WGPU_PLAT_MACOS
	Platform::Macos
#elif defined DVDBR3O_WGPU_PLAT_LINUX
	Platform::Linux
#elif defined DVDBR3O_WGPU_PLAT_ANDROID
	Platform::Android
#elif defined DVDBR3O_WGPU_PLAT_IOS
	Platform::Ios
#elif defined DVDBR3O_WGPU_PLAT_WASM
	Platform::Wasm
#else
	Platform::Unknown
#endif
	;

inline constexpr Mode mode =
#if defined DVDBR3O_WGPU_MODE_DEBUG
	Mode::Debug
#elif defined DVDBR3O_WGPU_MODE_RELEASE
	Mode::Release
#elif defined DVDBR3O_WGPU_MODE_PROFILE
	Mode::Profile
#else
	Mode::Release
#endif
	;
}  // namespace dvdbr3o