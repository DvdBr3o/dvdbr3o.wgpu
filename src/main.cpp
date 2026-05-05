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
#include <ft2build.h>
#include FT_FREETYPE_H
#include <webgpu/webgpu_cpp.h>
#include <spdlog/spdlog.h>
#include <stdexec/execution.hpp>
#include <exec/start_detached.hpp>
#include <ankerl/unordered_dense.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace dvdbr3o;
using exec::start_detached;
using exec::asio::asio_thread_pool;
using stdexec::continues_on;
using stdexec::just;
using stdexec::starts_on;
using stdexec::then;

namespace {
constexpr auto depth_format = wgpu::TextureFormat::Depth24PlusStencil8;
constexpr auto ascii_prepass_format = wgpu::TextureFormat::RGBA8Unorm;
constexpr auto ascii_depth_format = wgpu::TextureFormat::R16Float;
constexpr auto ascii_cell_width = 12;
constexpr auto ascii_cell_height = 18;
constexpr std::string_view ascii_glyphs = " .:-=+*#%@";

struct AsciiConfigData {
	int32_t viewport_width = 0;
	int32_t viewport_height = 0;
	int32_t cell_width = ascii_cell_width;
	int32_t cell_height = ascii_cell_height;
};

struct GlyphAtlasImage {
	std::vector<char> rgba;
	uint32_t		  width = 0;
	uint32_t		  height = 0;
};

inline auto surface_format_from(const Window& window) -> wgpu::TextureFormat {
	wgpu::SurfaceCapabilities caps;
	window.surface().GetCapabilities(Render::Context::global().adapter, &caps);
	return caps.formats[0];
}

inline auto render_pipeline_from_formats(
	Render::WgslSource shader_source, std::span<const wgpu::BindGroupLayout> bindgroup_layouts,
	std::span<const wgpu::TextureFormat> color_formats, const wgpu::DepthStencilState& depth_stencil,
	wgpu::CullMode cull_mode
) -> wgpu::RenderPipeline;

inline auto render_pipeline_from(
	Render::WgslSource shader_source, std::span<const wgpu::BindGroupLayout> bindgroup_layouts,
	wgpu::TextureFormat color_format, const wgpu::DepthStencilState& depth_stencil,
	wgpu::CullMode cull_mode = wgpu::CullMode::Back
) -> wgpu::RenderPipeline {
	return render_pipeline_from_formats(
		shader_source,
		bindgroup_layouts,
		std::span<const wgpu::TextureFormat>(&color_format, 1),
		depth_stencil,
		cull_mode
	);
}

inline auto render_pipeline_from_formats(
	Render::WgslSource shader_source, std::span<const wgpu::BindGroupLayout> bindgroup_layouts,
	std::span<const wgpu::TextureFormat> color_formats, const wgpu::DepthStencilState& depth_stencil,
	wgpu::CullMode cull_mode
) -> wgpu::RenderPipeline {
	const auto shader_module = Render::wgsl_module_from_source(shader_source);
	const wgpu::PipelineLayoutDescriptor pipeline_layout_desc {
		.bindGroupLayoutCount = bindgroup_layouts.size(),
		.bindGroupLayouts	  = bindgroup_layouts.data(),
	};
	const auto pipeline_layout =
		Render::Context::global().device.CreatePipelineLayout(&pipeline_layout_desc);
	std::vector<wgpu::ColorTargetState> color_target_states;
	color_target_states.reserve(color_formats.size());
	for (const auto format : color_formats)
		color_target_states.emplace_back(wgpu::ColorTargetState {
			.format = format,
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
			.module		 = shader_module,
			.bufferCount = 1,
			.buffers	 = &vertex_layout,
		},
		.primitive = {
			.frontFace = wgpu::FrontFace::CCW,
			.cullMode  = cull_mode,
		},
		.depthStencil = &depth_stencil,
		.fragment	  = &fragment,
	};
	return Render::Context::global().device.CreateRenderPipeline(&desc);
}

inline auto compute_pipeline_from(
	Render::WgslSource shader_source, std::span<const wgpu::BindGroupLayout> bindgroup_layouts
) -> wgpu::ComputePipeline {
	const auto shader_module = Render::wgsl_module_from_source(shader_source);
	const wgpu::PipelineLayoutDescriptor pipeline_layout_desc {
		.bindGroupLayoutCount = bindgroup_layouts.size(),
		.bindGroupLayouts	  = bindgroup_layouts.data(),
	};
	const auto pipeline_layout =
		Render::Context::global().device.CreatePipelineLayout(&pipeline_layout_desc);
	const wgpu::ComputePipelineDescriptor desc = {
		.layout = pipeline_layout,
		.compute = {
			.module = shader_module,
			.entryPoint = "computeMain",
		},
	};
	return Render::Context::global().device.CreateComputePipeline(&desc);
}

inline auto fullscreen_pipeline_from(
	Render::WgslSource shader_source, std::span<const wgpu::BindGroupLayout> bindgroup_layouts,
	wgpu::TextureFormat color_format, const wgpu::DepthStencilState& depth_stencil
) -> wgpu::RenderPipeline {
	const auto shader_module = Render::wgsl_module_from_source(shader_source);
	const wgpu::PipelineLayoutDescriptor pipeline_layout_desc {
		.bindGroupLayoutCount = bindgroup_layouts.size(),
		.bindGroupLayouts	  = bindgroup_layouts.data(),
	};
	const auto pipeline_layout =
		Render::Context::global().device.CreatePipelineLayout(&pipeline_layout_desc);
	const auto color_target_states = std::to_array<wgpu::ColorTargetState>({
		wgpu::ColorTargetState {
			.format = color_format,
		},
	});
	const wgpu::FragmentState fragment = {
		.module		 = shader_module,
		.entryPoint	 = "fragMain",
		.targetCount = color_target_states.size(),
		.targets	 = color_target_states.data(),
	};
	const wgpu::RenderPipelineDescriptor desc = {
		.layout = pipeline_layout,
		.vertex = {
			.module		= shader_module,
			.entryPoint	= "vertMain",
			.bufferCount = 0,
			.buffers	= nullptr,
		},
		.primitive = {
			.frontFace = wgpu::FrontFace::CCW,
			.cullMode  = wgpu::CullMode::None,
		},
		.depthStencil = &depth_stencil,
		.fragment	  = &fragment,
	};
	return Render::Context::global().device.CreateRenderPipeline(&desc);
}

inline auto rgba_texture(
	uint32_t width, uint32_t height, wgpu::TextureUsage usage,
	wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm
) -> wgpu::Texture {
	const wgpu::TextureDescriptor desc {
		.usage		   = usage,
		.dimension	   = wgpu::TextureDimension::e2D,
		.size		   = { width, height, 1 },
		.format		   = format,
		.mipLevelCount = 1,
		.sampleCount   = 1,
	};
	return Render::Context::global().device.CreateTexture(&desc);
}

inline auto generate_ascii_glyph_atlas(std::string_view glyphs) -> GlyphAtlasImage {
	FT_Library library = nullptr;
	if (FT_Init_FreeType(&library) != 0)
		throw std::runtime_error("failed to init FreeType");

	FT_Face face = nullptr;
#if defined(_WIN32)
	constexpr auto font_path = "C:\\Windows\\Fonts\\consola.ttf";
#else
	constexpr auto font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf";
#endif
	if (FT_New_Face(library, font_path, 0, &face) != 0) {
		FT_Done_FreeType(library);
		throw std::runtime_error("failed to load monospace font face");
	}

	FT_Set_Pixel_Sizes(face, 0, ascii_cell_height - 2);
	const auto cell_width = ascii_cell_width;
	const auto cell_height = ascii_cell_height;
	const auto atlas_width = static_cast<uint32_t>(glyphs.size() * cell_width);
	const auto atlas_height = static_cast<uint32_t>(cell_height);
	std::vector<char> atlas_rgba(static_cast<size_t>(atlas_width) * atlas_height * 4, 0);

	for (std::size_t i = 0; i < glyphs.size(); ++i) {
		if (FT_Load_Char(face, static_cast<unsigned long>(glyphs[i]), FT_LOAD_RENDER) != 0)
			continue;
		const auto& bitmap = face->glyph->bitmap;
		const int pen_x = static_cast<int>(i * cell_width) + 1;
		const int pen_y = static_cast<int>(face->size->metrics.ascender >> 6);
		for (unsigned int row = 0; row < bitmap.rows; ++row) {
			for (unsigned int col = 0; col < bitmap.width; ++col) {
				const int x = pen_x + static_cast<int>(face->glyph->bitmap_left) + static_cast<int>(col);
				const int y =
					pen_y - static_cast<int>(face->glyph->bitmap_top) + static_cast<int>(row);
				if (x < 0 || y < 0 || x >= static_cast<int>(atlas_width)
					|| y >= static_cast<int>(atlas_height))
					continue;
				const auto alpha = bitmap.buffer[row * bitmap.pitch + col];
				const auto idx =
					(static_cast<size_t>(y) * atlas_width + static_cast<size_t>(x)) * 4;
				atlas_rgba[idx + 0] = static_cast<char>(0xFF);
				atlas_rgba[idx + 1] = static_cast<char>(0xFF);
				atlas_rgba[idx + 2] = static_cast<char>(0xFF);
				atlas_rgba[idx + 3] = static_cast<char>(alpha);
			}
		}
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);
	return GlyphAtlasImage {
		.rgba = std::move(atlas_rgba),
		.width = atlas_width,
		.height = atlas_height,
	};
}
}  // namespace

struct AppState {
	using RenderPipelineMap =
		ankerl::unordered_dense::map<std::string_view, wgpu::RenderPipeline>;
	using RenderPassFn = std::function<void(wgpu::CommandEncoder&, const wgpu::TextureView&)>;
	using RenderPassMap = ankerl::unordered_dense::map<std::string_view, RenderPassFn>;

	Window					  window;
	Render::Global			  global;
	Render::Camera			  camera;
	Render::GlobalHandle	  global_handle;
	Render::CameraHandle	  camera_handle;
	asio_thread_pool		  asio_pool;
	bool					  is_left_dragging	 = false;
	bool					  is_middle_dragging = false;
	wgpu::Texture			  depth_texture;
	wgpu::Texture			  ascii_prepass_texture;
	wgpu::Texture			  ascii_depth_texture;
	wgpu::Texture			  glyph_atlas_texture;
	wgpu::Sampler			  glyph_sampler;
	wgpu::Buffer			  ascii_config_buffer;
	wgpu::Buffer			  ascii_cells_buffer;
	uint64_t				  ascii_cells_buffer_size = 0;
	RenderPipelineMap		  pipelines;
	RenderPassMap			  render_passes;
	wgpu::ComputePipeline	  ascii_compute_pipeline;
	wgpu::RenderPipeline	  ascii_composite_pipeline;
	std::array<wgpu::BindGroupLayout, 3> ascii_compute_bindgroup_layouts;
	std::array<wgpu::BindGroupLayout, 4> ascii_composite_bindgroup_layouts;
	wgpu::BindGroup			  ascii_compute_config_bg;
	wgpu::BindGroup			  ascii_compute_source_bg;
	wgpu::BindGroup			  ascii_compute_output_bg;
	wgpu::BindGroup			  ascii_composite_config_bg;
	wgpu::BindGroup			  ascii_composite_cells_bg;
	wgpu::BindGroup			  ascii_composite_glyphs_bg;
	wgpu::BindGroup			  ascii_composite_depth_bg;
	std::vector<Render::Mesh> meshes;
	uint64_t				  last_ticks = 0;

	//
	auto detach_on_asio(stdexec::sender auto&& snd) -> void {
		start_detached(starts_on(asio_pool.get_scheduler(), std::forward<decltype(snd)>(snd)));
	}

	auto recreate_depth_texture() -> void {
		const auto size = window.size();
		if (size.x <= 0 || size.y <= 0)
			return;

		const wgpu::TextureDescriptor desc {
			.usage	= wgpu::TextureUsage::RenderAttachment,
			.size	= { static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y), 1 },
			.format = depth_format,
		};
		depth_texture = Render::Context::global().device.CreateTexture(&desc);
	}

	auto recreate_ascii_targets() -> void {
		const auto size = window.size();
		if (size.x <= 0 || size.y <= 0)
			return;

		ascii_prepass_texture = rgba_texture(
			static_cast<uint32_t>(size.x),
			static_cast<uint32_t>(size.y),
			wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
			ascii_prepass_format
		);
		ascii_depth_texture = rgba_texture(
			static_cast<uint32_t>(size.x),
			static_cast<uint32_t>(size.y),
			wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
			ascii_depth_format
		);
		const auto cell_cols =
			static_cast<uint32_t>((size.x + ascii_cell_width - 1) / ascii_cell_width);
		const auto cell_rows =
			static_cast<uint32_t>((size.y + ascii_cell_height - 1) / ascii_cell_height);
		ascii_cells_buffer_size =
			static_cast<uint64_t>(cell_cols) * cell_rows * sizeof(glm::vec4);
		ascii_cells_buffer =
			Render::create_buffer<wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst>(
				ascii_cells_buffer_size
			);
	}

	auto update_ascii_config() const -> void {
		const auto size = window.size();
		const AsciiConfigData config {
			.viewport_width = size.x,
			.viewport_height = size.y,
		};
		Render::Context::global().queue.WriteBuffer(ascii_config_buffer, 0, &config, sizeof(config));
	}

	auto refresh_ascii_bindgroups(
		const wgpu::BindGroupLayout& compute_config_layout,
		const wgpu::BindGroupLayout& compute_source_layout,
		const wgpu::BindGroupLayout& compute_output_layout,
		const wgpu::BindGroupLayout& composite_config_layout,
		const wgpu::BindGroupLayout& composite_cells_layout,
		const wgpu::BindGroupLayout& composite_glyphs_layout,
		const wgpu::BindGroupLayout& composite_depth_layout
	) -> void {
		ascii_compute_config_bg = Render::bindgroup_from(
			compute_config_layout,
			std::array {
				wgpu::BindGroupEntry {
					.binding = 0,
					.buffer = ascii_config_buffer,
					.size = sizeof(AsciiConfigData),
				},
			}
		);
		ascii_compute_source_bg = Render::bindgroup_from(
			compute_source_layout,
			std::array {
				wgpu::BindGroupEntry {
					.binding = 0,
					.textureView = ascii_prepass_texture.CreateView(),
				},
			}
		);
		ascii_compute_output_bg = Render::bindgroup_from(
			compute_output_layout,
			std::array {
				wgpu::BindGroupEntry {
					.binding = 0,
					.buffer = ascii_cells_buffer,
					.size = ascii_cells_buffer_size,
				},
			}
		);
		ascii_composite_config_bg = Render::bindgroup_from(
			composite_config_layout,
			std::array {
				wgpu::BindGroupEntry {
					.binding = 0,
					.buffer = ascii_config_buffer,
					.size = sizeof(AsciiConfigData),
				},
			}
		);
		ascii_composite_cells_bg = Render::bindgroup_from(
			composite_cells_layout,
			std::array {
				wgpu::BindGroupEntry {
					.binding = 0,
					.buffer = ascii_cells_buffer,
					.size = ascii_cells_buffer_size,
				},
			}
		);
		ascii_composite_glyphs_bg = Render::bindgroup_from(
			composite_glyphs_layout,
			std::array {
				wgpu::BindGroupEntry {
					.binding = 0,
					.textureView = glyph_atlas_texture.CreateView(),
				},
				wgpu::BindGroupEntry {
					.binding = 1,
					.sampler = glyph_sampler,
				},
			}
		);
		ascii_composite_depth_bg = Render::bindgroup_from(
			composite_depth_layout,
			std::array {
				wgpu::BindGroupEntry {
					.binding = 0,
					.textureView = ascii_depth_texture.CreateView(),
				},
			}
		);
	}

	auto render_meshes(
		wgpu::CommandEncoder& cmd, const wgpu::TextureView& color_view,
		std::string_view pipeline_label, const wgpu::RenderPipeline& pipeline,
		wgpu::LoadOp color_load_op, wgpu::LoadOp depth_load_op, wgpu::LoadOp stencil_load_op,
		uint32_t stencil_reference
	) const -> void {
		const wgpu::RenderPassColorAttachment color_attachment = {
			.view		= color_view,
			.loadOp		= color_load_op,
			.storeOp	= wgpu::StoreOp::Store,
			.clearValue = { 0.f, 0.f, 0.f, 0.f },
		};
		const wgpu::RenderPassDepthStencilAttachment depth_attachment = {
			.view			   = depth_texture.CreateView(),
			.depthLoadOp	   = depth_load_op,
			.depthStoreOp	   = wgpu::StoreOp::Store,
			.depthClearValue   = 1.0f,
			.stencilLoadOp	   = stencil_load_op,
			.stencilStoreOp	   = wgpu::StoreOp::Store,
			.stencilClearValue = 0,
		};
		const wgpu::RenderPassDescriptor desc {
			.colorAttachmentCount	= 1,
			.colorAttachments		= &color_attachment,
			.depthStencilAttachment = &depth_attachment,
		};
		auto pass = cmd.BeginRenderPass(&desc);
		pass.SetPipeline(pipeline);
		pass.SetBindGroup(0, global_handle.bindgroup);
		pass.SetBindGroup(1, camera_handle.bindgroup);
		pass.SetStencilReference(stencil_reference);
		for (const auto& mesh : meshes) {
			if (mesh.pipeline != pipeline_label)
				continue;
			mesh.render(pass);
		}
		pass.End();
	}

	auto dispatch_ascii_compute(wgpu::CommandEncoder& cmd) const -> void {
		auto pass = cmd.BeginComputePass();
		pass.SetPipeline(ascii_compute_pipeline);
		pass.SetBindGroup(0, ascii_compute_config_bg);
		pass.SetBindGroup(1, ascii_compute_source_bg);
		pass.SetBindGroup(2, ascii_compute_output_bg);
		const auto size = window.size();
		const uint32_t cell_cols =
			static_cast<uint32_t>((size.x + ascii_cell_width - 1) / ascii_cell_width);
		const uint32_t cell_rows =
			static_cast<uint32_t>((size.y + ascii_cell_height - 1) / ascii_cell_height);
		pass.DispatchWorkgroups((cell_cols + 7) / 8, (cell_rows + 7) / 8, 1);
		pass.End();
	}
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
	spdlog::info("hello from dvdbr3o.wgpu!");

	const auto mtoon_bindgroup_layouts =
		Render::bindgroup_layouts_from_string(Shaders::mtoon.layout_json);
	const auto ascii_bindgroup_layouts =
		Render::bindgroup_layouts_from_string(Shaders::ascii.layout_json);

	*appstate = new AppState {
		.window		   = { {
				   .width  = 1080,
				   .height = 720,
				   .title  = "dvdbr3o",
		   } },
		.global_handle = Render::GlobalHandle::from(mtoon_bindgroup_layouts[0]),
		.camera_handle = Render::CameraHandle::from(mtoon_bindgroup_layouts[1]),
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
	app.recreate_depth_texture();
	app.recreate_ascii_targets();
	app.last_ticks = SDL_GetTicks();
	app.global_handle.update(app.global);
	app.camera_handle.update(app.camera);
	app.ascii_config_buffer =
		Render::create_buffer<wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst>(
			sizeof(AsciiConfigData)
		);
	app.update_ascii_config();

	{
		auto atlas = generate_ascii_glyph_atlas(ascii_glyphs);
		app.glyph_atlas_texture = Render::texture_from_image(
			Render::ImageInfo {
				.data = std::span<char> { atlas.rgba.data(), atlas.rgba.size() },
				.width = atlas.width,
				.height = atlas.height,
			}
		);
		app.glyph_sampler = Render::isotropic_sampler(
			wgpu::AddressMode::ClampToEdge,
			wgpu::FilterMode::Nearest
		);
	}

	const auto color_format	   = surface_format_from(app.window);
	const auto stencil_replace = wgpu::StencilFaceState {
		.compare	 = wgpu::CompareFunction::Always,
		.failOp		 = wgpu::StencilOperation::Keep,
		.depthFailOp = wgpu::StencilOperation::Keep,
		.passOp		 = wgpu::StencilOperation::Replace,
	};
	const auto stencil_keep = wgpu::StencilFaceState {
		.compare	 = wgpu::CompareFunction::Always,
		.failOp		 = wgpu::StencilOperation::Keep,
		.depthFailOp = wgpu::StencilOperation::Keep,
		.passOp		 = wgpu::StencilOperation::Keep,
	};
	app.pipelines[Render::MeshPipelineLabel::mtoon] = render_pipeline_from(
		Shaders::mtoon.wgsl,
		mtoon_bindgroup_layouts,
		color_format,
		wgpu::DepthStencilState {
			.format			   = depth_format,
			.depthWriteEnabled = true,
			.depthCompare	   = wgpu::CompareFunction::Less,
			.stencilFront	   = stencil_keep,
			.stencilBack	   = stencil_keep,
		}
	);
	app.pipelines[Render::MeshPipelineLabel::ascii] = render_pipeline_from_formats(
		Shaders::ascii.wgsl,
		ascii_bindgroup_layouts,
		std::to_array<wgpu::TextureFormat>({ ascii_prepass_format, ascii_depth_format }),
		wgpu::DepthStencilState {
			.format			   = depth_format,
			.depthWriteEnabled = false,
			.depthCompare	   = wgpu::CompareFunction::Always,
			.stencilFront	   = stencil_replace,
			.stencilBack	   = stencil_replace,
			.stencilWriteMask  = 0xFF,
		},
		wgpu::CullMode::None
	);
	const auto compute_config_entries = std::to_array<wgpu::BindGroupLayoutEntry>({
		wgpu::BindGroupLayoutEntry {
			.binding = 0,
			.visibility = wgpu::ShaderStage::Compute | wgpu::ShaderStage::Fragment,
			.buffer = {
				.type = wgpu::BufferBindingType::Uniform,
				.minBindingSize = sizeof(AsciiConfigData),
			},
		},
	});
	const wgpu::BindGroupLayoutDescriptor compute_config_desc {
		.entryCount = compute_config_entries.size(),
		.entries = compute_config_entries.data(),
	};
	const auto compute_source_entries = std::to_array<wgpu::BindGroupLayoutEntry>({
		wgpu::BindGroupLayoutEntry {
			.binding = 0,
			.visibility = wgpu::ShaderStage::Compute,
			.texture = {
				.sampleType = wgpu::TextureSampleType::Float,
				.viewDimension = wgpu::TextureViewDimension::e2D,
			},
		},
	});
	const wgpu::BindGroupLayoutDescriptor compute_source_desc {
		.entryCount = compute_source_entries.size(),
		.entries = compute_source_entries.data(),
	};
	const auto compute_output_entries = std::to_array<wgpu::BindGroupLayoutEntry>({
		wgpu::BindGroupLayoutEntry {
			.binding = 0,
			.visibility = wgpu::ShaderStage::Compute,
			.buffer = {
				.type = wgpu::BufferBindingType::Storage,
				.minBindingSize = 0,
			},
		},
	});
	const wgpu::BindGroupLayoutDescriptor compute_output_desc {
		.entryCount = compute_output_entries.size(),
		.entries = compute_output_entries.data(),
	};
	app.ascii_compute_bindgroup_layouts = std::to_array<wgpu::BindGroupLayout>({
		Render::Context::global().device.CreateBindGroupLayout(&compute_config_desc),
		Render::Context::global().device.CreateBindGroupLayout(&compute_source_desc),
		Render::Context::global().device.CreateBindGroupLayout(&compute_output_desc),
	});

	const auto composite_cells_entries = std::to_array<wgpu::BindGroupLayoutEntry>({
		wgpu::BindGroupLayoutEntry {
			.binding = 0,
			.visibility = wgpu::ShaderStage::Fragment,
			.buffer = {
				.type = wgpu::BufferBindingType::ReadOnlyStorage,
				.minBindingSize = 0,
			},
		},
	});
	const wgpu::BindGroupLayoutDescriptor composite_cells_desc {
		.entryCount = composite_cells_entries.size(),
		.entries = composite_cells_entries.data(),
	};
	const auto composite_glyph_entries = std::to_array<wgpu::BindGroupLayoutEntry>({
		wgpu::BindGroupLayoutEntry {
			.binding = 0,
			.visibility = wgpu::ShaderStage::Fragment,
			.texture = {
				.sampleType = wgpu::TextureSampleType::Float,
				.viewDimension = wgpu::TextureViewDimension::e2D,
			},
		},
		wgpu::BindGroupLayoutEntry {
			.binding = 1,
			.visibility = wgpu::ShaderStage::Fragment,
			.sampler = {
				.type = wgpu::SamplerBindingType::Filtering,
			},
		},
	});
	const wgpu::BindGroupLayoutDescriptor composite_glyph_desc {
		.entryCount = composite_glyph_entries.size(),
		.entries = composite_glyph_entries.data(),
	};
	const auto composite_depth_entries = std::to_array<wgpu::BindGroupLayoutEntry>({
		wgpu::BindGroupLayoutEntry {
			.binding = 0,
			.visibility = wgpu::ShaderStage::Fragment,
			.texture = {
				.sampleType = wgpu::TextureSampleType::UnfilterableFloat,
				.viewDimension = wgpu::TextureViewDimension::e2D,
			},
		},
	});
	const wgpu::BindGroupLayoutDescriptor composite_depth_desc {
		.entryCount = composite_depth_entries.size(),
		.entries = composite_depth_entries.data(),
	};
	app.ascii_composite_bindgroup_layouts = std::to_array<wgpu::BindGroupLayout>({
		app.ascii_compute_bindgroup_layouts[0],
		Render::Context::global().device.CreateBindGroupLayout(&composite_cells_desc),
		Render::Context::global().device.CreateBindGroupLayout(&composite_glyph_desc),
		Render::Context::global().device.CreateBindGroupLayout(&composite_depth_desc),
	});
	app.ascii_compute_pipeline =
		compute_pipeline_from(Shaders::ascii_compute.wgsl, app.ascii_compute_bindgroup_layouts);
	app.ascii_composite_pipeline = fullscreen_pipeline_from(
		Shaders::ascii_composite.wgsl,
		app.ascii_composite_bindgroup_layouts,
		color_format,
		wgpu::DepthStencilState {
			.format			   = depth_format,
			.depthWriteEnabled = true,
			.depthCompare	   = wgpu::CompareFunction::Always,
			.stencilFront = {
				.compare	 = wgpu::CompareFunction::Equal,
				.failOp		 = wgpu::StencilOperation::Keep,
				.depthFailOp = wgpu::StencilOperation::Keep,
				.passOp		 = wgpu::StencilOperation::Keep,
			},
			.stencilBack = {
				.compare	 = wgpu::CompareFunction::Equal,
				.failOp		 = wgpu::StencilOperation::Keep,
				.depthFailOp = wgpu::StencilOperation::Keep,
				.passOp		 = wgpu::StencilOperation::Keep,
			},
			.stencilReadMask = 0xFF,
		}
	);

	app.meshes = Render::model_from(Models::lab_vrm, mtoon_bindgroup_layouts[2]);
	app.refresh_ascii_bindgroups(
		app.ascii_compute_bindgroup_layouts[0],
		app.ascii_compute_bindgroup_layouts[1],
		app.ascii_compute_bindgroup_layouts[2],
		app.ascii_composite_bindgroup_layouts[0],
		app.ascii_composite_bindgroup_layouts[1],
		app.ascii_composite_bindgroup_layouts[2],
		app.ascii_composite_bindgroup_layouts[3]
	);
	app.render_passes["ascii_prepass"] = [&app](wgpu::CommandEncoder& cmd, const wgpu::TextureView&) {
		const auto color_attachments = std::to_array<wgpu::RenderPassColorAttachment>({
			wgpu::RenderPassColorAttachment {
				.view = app.ascii_prepass_texture.CreateView(),
				.loadOp = wgpu::LoadOp::Clear,
				.storeOp = wgpu::StoreOp::Store,
				.clearValue = { 0.f, 0.f, 0.f, 0.f },
			},
			wgpu::RenderPassColorAttachment {
				.view = app.ascii_depth_texture.CreateView(),
				.loadOp = wgpu::LoadOp::Clear,
				.storeOp = wgpu::StoreOp::Store,
				.clearValue = { 1.f, 0.f, 0.f, 0.f },
			},
		});
		const wgpu::RenderPassDepthStencilAttachment depth_attachment = {
			.view			   = app.depth_texture.CreateView(),
			.depthLoadOp	   = wgpu::LoadOp::Clear,
			.depthStoreOp	   = wgpu::StoreOp::Discard,
			.depthClearValue   = 1.0f,
			.stencilLoadOp	   = wgpu::LoadOp::Clear,
			.stencilStoreOp	   = wgpu::StoreOp::Store,
			.stencilClearValue = 0,
		};
		const wgpu::RenderPassDescriptor desc {
			.colorAttachmentCount = color_attachments.size(),
			.colorAttachments = color_attachments.data(),
			.depthStencilAttachment = &depth_attachment,
		};
		auto pass = cmd.BeginRenderPass(&desc);
		pass.SetPipeline(app.pipelines.at(Render::MeshPipelineLabel::ascii));
		pass.SetBindGroup(0, app.global_handle.bindgroup);
		pass.SetBindGroup(1, app.camera_handle.bindgroup);
		pass.SetStencilReference(1);
		for (const auto& mesh : app.meshes) {
			if (mesh.pipeline != Render::MeshPipelineLabel::ascii)
				continue;
			mesh.render(pass);
		}
		pass.End();
	};
	app.render_passes["mtoon"] = [&app](wgpu::CommandEncoder& cmd, const wgpu::TextureView& color_view) {
		app.render_meshes(
			cmd,
			color_view,
			Render::MeshPipelineLabel::mtoon,
			app.pipelines.at(Render::MeshPipelineLabel::mtoon),
			wgpu::LoadOp::Clear,
			wgpu::LoadOp::Clear,
			wgpu::LoadOp::Load,
			0
		);
	};
	app.render_passes["ascii_composite"] = [&app](wgpu::CommandEncoder& cmd, const wgpu::TextureView& color_view) {
		app.dispatch_ascii_compute(cmd);

		const wgpu::RenderPassColorAttachment color_attachment = {
			.view = color_view,
			.loadOp = wgpu::LoadOp::Load,
			.storeOp = wgpu::StoreOp::Store,
		};
		const wgpu::RenderPassDepthStencilAttachment depth_attachment = {
			.view = app.depth_texture.CreateView(),
			.depthLoadOp = wgpu::LoadOp::Load,
			.depthStoreOp = wgpu::StoreOp::Store,
			.depthClearValue = 1.0f,
			.stencilLoadOp = wgpu::LoadOp::Load,
			.stencilStoreOp = wgpu::StoreOp::Store,
			.stencilClearValue = 0,
		};
		const wgpu::RenderPassDescriptor desc {
			.colorAttachmentCount = 1,
			.colorAttachments = &color_attachment,
			.depthStencilAttachment = &depth_attachment,
		};
		auto pass = cmd.BeginRenderPass(&desc);
		pass.SetPipeline(app.ascii_composite_pipeline);
		pass.SetBindGroup(0, app.ascii_composite_config_bg);
		pass.SetBindGroup(1, app.ascii_composite_cells_bg);
		pass.SetBindGroup(2, app.ascii_composite_glyphs_bg);
		pass.SetBindGroup(3, app.ascii_composite_depth_bg);
		pass.SetStencilReference(1);
		pass.Draw(3);
		pass.End();
	};

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	// Render
	auto&		app		  = *reinterpret_cast<AppState*>(appstate);
	const auto& render	  = Render::Context::global();
	const auto	now_ticks = SDL_GetTicks();
	app.global.delta_time = static_cast<float>(now_ticks - app.last_ticks) / 1000.0f;
	app.global.time += app.global.delta_time;
	app.global.frame += 1.0f;
	app.last_ticks = now_ticks;
	app.global_handle.update(app.global);
	app.camera_handle.update(app.camera);

	wgpu::SurfaceTexture tex_present;
	app.window.surface().GetCurrentTexture(&tex_present);

	auto	   cmd		  = Render::Context::global().device.CreateCommandEncoder();
	const auto color_view = tex_present.texture.CreateView();
	constexpr auto pass_order =
		std::to_array<std::string_view>({ "ascii_prepass", "mtoon", "ascii_composite" });
	for (const auto label : pass_order)
		app.render_passes.at(label)(cmd, color_view);

	const auto submit = cmd.Finish();
	render.queue.Submit(1, &submit);

	app.window.surface().Present();
	render.instance.ProcessEvents();

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
				app.recreate_depth_texture();
				app.recreate_ascii_targets();
				app.update_ascii_config();
				app.refresh_ascii_bindgroups(
					app.ascii_compute_bindgroup_layouts[0],
					app.ascii_compute_bindgroup_layouts[1],
					app.ascii_compute_bindgroup_layouts[2],
					app.ascii_composite_bindgroup_layouts[0],
					app.ascii_composite_bindgroup_layouts[1],
					app.ascii_composite_bindgroup_layouts[2],
					app.ascii_composite_bindgroup_layouts[3]
				);
			}
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			if (event->button.windowID != app.window.id())
				break;
			if (event->button.button == SDL_BUTTON_LEFT)
				app.is_left_dragging = true;
			else if (event->button.button == SDL_BUTTON_MIDDLE)
				app.is_middle_dragging = true;
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (event->button.windowID != app.window.id())
				break;
			if (event->button.button == SDL_BUTTON_LEFT)
				app.is_left_dragging = false;
			else if (event->button.button == SDL_BUTTON_MIDDLE)
				app.is_middle_dragging = false;
			break;
		case SDL_EVENT_MOUSE_MOTION:
			if (event->motion.windowID == app.window.id()) {
				const auto size = app.window.size();
				if (app.is_left_dragging)
					app.camera
						.rotate_from_mouse(event->motion.xrel, event->motion.yrel, size.x, size.y);
				if (app.is_middle_dragging)
					app.camera
						.pan_from_mouse(event->motion.xrel, event->motion.yrel, size.x, size.y);
			}
			break;
		case SDL_EVENT_MOUSE_WHEEL:
			if (event->wheel.windowID == app.window.id())
				app.camera.dolly_from_wheel(event->wheel.y);
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
