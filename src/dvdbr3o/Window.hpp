#pragma once

#include "dvdbr3o/Render/Utils.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <webgpu/webgpu_cpp.h>

#include <string_view>

namespace dvdbr3o {
struct WindowSpec {
	int				 width;
	int				 height;
	std::string_view title;
};

class Window {
	SDL_Window*	  _window;
	wgpu::Surface _surface;

private:
	auto _configure_surface() -> void {
		const auto size = this->size();
		if (size.x <= 0 || size.y <= 0)
			return;

		wgpu::SurfaceCapabilities caps {};
		_surface.GetCapabilities(Render::Context::global().adapter, &caps);

		const wgpu::SurfaceConfiguration config {
			.device		 = Render::Context::global().device,
			.format		 = caps.formats[0],
			.width		 = static_cast<uint32_t>(size.x),
			.height		 = static_cast<uint32_t>(size.y),
			.alphaMode	 = caps.alphaModes[0],
			.presentMode = caps.presentModes[0],
		};
		_surface.Configure(&config);
	}

public:
	using Spec = WindowSpec;

public:
	Window(const Spec& spec) :
		_window(
			SDL_CreateWindow(spec.title.data(), spec.width, spec.height, SDL_WINDOW_RESIZABLE)
		) {
		SDL_ShowWindow(_window);
		_surface = Render::create_sdl3_surface(_window);
		_configure_surface();
	}

	[[nodiscard]] auto sdl() const -> SDL_Window* { return _window; }

	[[nodiscard]] auto id() const -> SDL_WindowID { return SDL_GetWindowID(_window); }

	[[nodiscard]] auto size() const -> SDL_Point {
		SDL_Point size {};
		SDL_GetWindowSize(_window, &size.x, &size.y);
		return size;
	}

	[[nodiscard]] auto surface() -> wgpu::Surface& { return _surface; }

	[[nodiscard]] auto surface() const -> const wgpu::Surface& { return _surface; }

	auto			   resize_surface() -> void { _configure_surface(); }

public:
	auto poll() -> void {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT
				|| (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
					&& event.window.windowID == SDL_GetWindowID(_window))) {
				SDL_DestroyWindow(_window);
				_window = nullptr;
				return;
			}
		}
	}

	auto launch() -> void {
		while (_window != nullptr) poll();
	}
};
}  // namespace dvdbr3o
