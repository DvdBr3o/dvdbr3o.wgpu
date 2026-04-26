#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "dvdbr3o/Render/Context.hpp"
#include "dvdbr3o/Render/Model.hpp"
#include "dvdbr3o/Render/Pipeline.hpp"
#include "dvdbr3o/Render/Shader.hpp"
#include "dvdbr3o/Shaders/Embed.hpp"
#include "dvdbr3o/Render/Camera.hpp"
#include "dvdbr3o/Utils.hpp"
#include "dvdbr3o/Window.hpp"
#include "dvdbr3o/Render/Vertice.hpp"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>
#include <webgpu/webgpu_cpp.h>
#include <spdlog/spdlog.h>
#include <stdexec/execution.hpp>
#include <exec/start_detached.hpp>

#include <array>
#include <filesystem>

using namespace dvdbr3o;
using exec::start_detached;
using exec::asio::asio_thread_pool;
using stdexec::continues_on;
using stdexec::just;
using stdexec::starts_on;
using stdexec::then;

struct AppState {
	Window					  window;
	Render::Global			  global;
	Render::Camera			  camera;
	Render::GlobalHandle	  global_handle;
	Render::CameraHandle	  camera_handle;
	asio_thread_pool		  asio_pool;
	bool					  is_dragging = false;
	wgpu::RenderPipeline	  pipeline;
	std::vector<Render::Mesh> meshes;
	uint64_t				  last_ticks = 0;

	//
	auto detach_on_asio(stdexec::sender auto&& snd) -> void {
		start_detached(starts_on(asio_pool.get_scheduler(), std::forward<decltype(snd)>(snd)));
	}
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	spdlog::info("hello from dvdbr3o.wgpu!");

	const auto bindgroup_layouts =
		Render::bindgroup_layouts_from_string(Shaders::mtoon.layout_json);

	*appstate = new AppState {
		.window		   = { {
				   .width  = 1080,
				   .height = 720,
				   .title  = "dvdbr3o",
		   } },
		.global_handle = Render::GlobalHandle::from(bindgroup_layouts[0]),
		.camera_handle = Render::CameraHandle::from(bindgroup_layouts[1]),
	};

	auto& app = *reinterpret_cast<AppState*>(*appstate);
	{
		const auto size = app.window.size();
		app.camera.update_aspect(size.x, size.y);
		app.global.viewport = {
			.width	= size.x,
			.height = size.y,
		};
	}
	app.last_ticks = SDL_GetTicks();
	app.global_handle.update(app.global);
	app.camera_handle.update(app.camera);

	// const auto layout = Render::layout_of_parsed(Shaders::ascii.layout_json);
	// Render::model_from(Models::vrm1_constraint_twist_sample_gltf);

	app.pipeline = [&]() {
		const auto shader_module = Render::wgsl_module_from_source(Shaders::mtoon.wgsl);
		const wgpu::PipelineLayoutDescriptor pipeline_layout_desc {
			.bindGroupLayoutCount = bindgroup_layouts.size(),
			.bindGroupLayouts	  = bindgroup_layouts.data(),
		};
		const auto pipeline_layout =
			Render::Context::global().device.CreatePipelineLayout(&pipeline_layout_desc);
		const auto				  color_target_states = std::to_array<wgpu::ColorTargetState>({
			   wgpu::ColorTargetState {
									//
							   .format =
					   [&]() {
						   wgpu::SurfaceCapabilities caps;
						   app.window.surface().GetCapabilities(
							   Render::Context::global().adapter,
							   &caps
						   );
						   return caps.formats[0];
					   }(),
									},
		   });
		const wgpu::FragmentState fragment			  = {
					   .module		= shader_module,
					   .targetCount = color_target_states.size(),
					   .targets		= color_target_states.data(),
		};
		constexpr auto				   vertex_attrib = Render::Vertice::vertex_attribute();
		const wgpu::VertexBufferLayout vertex_layout = {
			.stepMode		= wgpu::VertexStepMode::Vertex,
			.arrayStride	= sizeof(Render::Vertice),
			.attributeCount = vertex_attrib.size(),
			.attributes		= vertex_attrib.data(),
		};
		const wgpu::RenderPipelineDescriptor desc = {
			.layout = pipeline_layout,
			.vertex = {
				.module			 = shader_module,
				.bufferCount = 1,
				.buffers = &vertex_layout,
			},
			.primitive = {
				.frontFace = wgpu::FrontFace::CCW,
				.cullMode = wgpu::CullMode::Back,
			},
			.fragment = &fragment,
		};
		return Render::Context::global().device.CreateRenderPipeline(&desc);
	}();

	app.meshes =
		Render::model_from(Models::vrm1_constraint_twist_sample_gltf, bindgroup_layouts[2]);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	// Render
	auto&	   app		  = *reinterpret_cast<AppState*>(appstate);
	const auto now_ticks  = SDL_GetTicks();
	app.global.delta_time = static_cast<float>(now_ticks - app.last_ticks) / 1000.0f;
	app.global.time += app.global.delta_time;
	app.global.frame += 1.0f;
	app.last_ticks = now_ticks;
	app.global_handle.update(app.global);
	app.camera_handle.update(app.camera);

	wgpu::SurfaceTexture tex_present;
	app.window.surface().GetCurrentTexture(&tex_present);

	auto cmd  = Render::Context::global().device.CreateCommandEncoder();
	auto pass = [&]() {
		const wgpu::RenderPassColorAttachment color_attachment = {
			.view		= tex_present.texture.CreateView(),
			.loadOp		= wgpu::LoadOp::Clear,
			.storeOp	= wgpu::StoreOp::Store,
			.clearValue = { 0.1f, 0.8f, 0.9f, 1.0f },
		};
		const wgpu::RenderPassDescriptor desc {
			.colorAttachmentCount = 1,
			.colorAttachments	  = &color_attachment,
		};
		return cmd.BeginRenderPass(&desc);
	}();

	pass.SetPipeline(app.pipeline);
	pass.SetBindGroup(0, app.global_handle.bindgroup);
	pass.SetBindGroup(1, app.camera_handle.bindgroup);
	for (const auto& mesh : app.meshes) mesh.render(pass);
	pass.End();

	const auto submit = cmd.Finish();
	Render::Context::global().queue.Submit(1, &submit);

	app.window.surface().Present();
	Render::Context::global().instance.ProcessEvents();

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	auto& app = *reinterpret_cast<AppState*>(appstate);

	switch (event->type) {
		case SDL_EVENT_WINDOW_CLOSE_REQUESTED: return SDL_APP_SUCCESS;
		case SDL_EVENT_WINDOW_RESIZED:
			if (event->window.windowID == app.window.id()) {
				app.camera.update_aspect(event->window.data1, event->window.data2);
				app.global.viewport = {
					.width	= event->window.data1,
					.height = event->window.data2,
				};
				app.window.resize_surface();
			}
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			if (event->button.windowID == app.window.id()
				&& event->button.button == SDL_BUTTON_LEFT)
				app.is_dragging = true;
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (event->button.windowID == app.window.id()
				&& event->button.button == SDL_BUTTON_LEFT)
				app.is_dragging = false;
			break;
		case SDL_EVENT_MOUSE_MOTION:
			if (event->motion.windowID == app.window.id() && app.is_dragging) {
				const auto size = app.window.size();
				// spdlog::info("rotating!");
				app.camera
					.rotate_from_mouse(event->motion.xrel, event->motion.yrel, size.x, size.y);
			}

			break;
		case SDL_EVENT_KEY_DOWN:
			if (event->key.windowID != app.window.id())
				break;
			if (event->key.key == SDLK_ESCAPE)
				return SDL_APP_SUCCESS;
			break;
		default: break;
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	spdlog::info("bye from dvdbr3o.wgpu!");
	delete reinterpret_cast<AppState*>(appstate);
}
