#pragma once

#include "dvdbr3o/Render/Context.hpp"
#include "dvdbr3o/Stb.hpp"

#include <webgpu/webgpu_cpp.h>
#include <SDL3/SDL_platform.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <fastgltf/types.hpp>

#if defined SDL_PLATFORM_WINDOWS
#	include <Windows.h>
#endif

#include <memory>

namespace dvdbr3o::Render {
using ManagedChainedStruct = std::unique_ptr<wgpu::ChainedStruct, void (*)(wgpu::ChainedStruct*)>;

namespace details::sdl3_surface {
inline auto _sdl3_surface_chained_desc(SDL_Window* window, Context& ctx = Context::global())
	-> ManagedChainedStruct {
#if defined SDL_PLATFORM_WINDOWS
	const auto props = SDL_GetWindowProperties(window);
	auto*	   desc	 = new wgpu::SurfaceSourceWindowsHWND {};
	desc->hwnd		 = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
	desc->hinstance	 = GetModuleHandle(nullptr);

	return ManagedChainedStruct { desc, [](wgpu::ChainedStruct* desc) {
									 delete reinterpret_cast<wgpu::SurfaceSourceWindowsHWND*>(desc);
								 } };
#elif defined(DAWN_USE_WAYLAND) || defined(DAWN_USE_X11)
#	if defined(GLFW_PLATFORM_WAYLAND) && defined(DAWN_USE_WAYLAND)
	if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
		wgpu::SurfaceSourceWaylandSurface* desc = new wgpu::SurfaceSourceWaylandSurface();
		desc->display							= glfwGetWaylandDisplay();
		desc->surface							= glfwGetWaylandWindow(window);
		return { desc, [](wgpu::ChainedStruct* desc) {
					delete reinterpret_cast<wgpu::SurfaceSourceWaylandSurface*>(desc);
				} };
	} else	// NOLINT(readability/braces)
#	endif
#	if defined(DAWN_USE_X11)
	{
		wgpu::SurfaceSourceXlibWindow* desc = new wgpu::SurfaceSourceXlibWindow();
		desc->display						= glfwGetX11Display();
		desc->window						= glfwGetX11Window(window);
		return { desc, [](wgpu::ChainedStruct* desc) {
					delete reinterpret_cast<wgpu::SurfaceSourceXlibWindow*>(desc);
				} };
	}
#	else
	{
		return { nullptr, [](wgpu::ChainedStruct*) {} };
	}
#	endif
#else
	// TODO: More Platforms
	panic("unexpected platform when creating sdl3 surface!");
#endif
}

inline auto create_sdl3_surface(SDL_Window* window, Context& ctx = Context::global())
	-> wgpu::Surface {
	const auto					  chained = _sdl3_surface_chained_desc(window, ctx);
	const wgpu::SurfaceDescriptor desc	  = {
		   .nextInChain = chained.get(),
	};
	return ctx.instance.CreateSurface(&desc);
}

inline auto create_sdl3_surface(Context& ctx, SDL_Window* window) -> wgpu::Surface {
	return create_sdl3_surface(window, ctx);
}
}  // namespace details::sdl3_surface

using details::sdl3_surface::create_sdl3_surface;

struct ImageInfo {
	std::span<char> data;
	uint32_t		width;
	uint32_t		height;
};

inline auto texture_from_image(
	const ImageInfo& image, const Context& ctx = Context::global(),
	wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm
) -> wgpu::Texture {
	const wgpu::TextureDescriptor desc {
		.usage		   = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst,
		.dimension	   = wgpu::TextureDimension::e2D,
		.size		   = { image.width, image.height, 1 },
		.format		   = format,
		.mipLevelCount = 1,
		.sampleCount   = 1,
	};
	wgpu::Texture					 texture = ctx.device.CreateTexture(&desc);

	const wgpu::TexelCopyTextureInfo dest {
		.texture  = texture,
		.mipLevel = 0,
		.aspect	  = wgpu::TextureAspect::All,
	};
	const wgpu::TexelCopyBufferLayout layout {
		.offset		  = 0,
		.bytesPerRow  = image.width * 4,
		.rowsPerImage = image.height,
	};
	ctx.queue.WriteTexture(
		&dest,
		image.data.data(),
		image.width * image.height * 4,
		&layout,
		&desc.size
	);

	return texture;
}

inline auto texture_from_image(
	const Context& ctx, const ImageInfo& image,
	wgpu::TextureFormat format = wgpu::TextureFormat::RGBA8Unorm
) -> wgpu::Texture {
	return texture_from_image(image, ctx, format);
}

inline auto texture_from_image(
	const fastgltf::Asset& asset, const fastgltf::Image& image, Context& ctx = Context::global()
) -> wgpu::Texture {
	wgpu::Texture texture;
	std::visit(
		fastgltf::visitor {
			[&](const fastgltf::sources::URI& filePath) {
				assert(filePath.fileByteOffset == 0);  // We don't support offsets with stbi.
				assert(filePath.uri.isLocalPath());	   // We're only capable of loading local files.
				int				  width, height, nrChannels;

				const std::string path(
					filePath.uri.path().begin(),
					filePath.uri.path().end()
				);	// Thanks C++.
				unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);

				texture				= texture_from_image(
					ctx,
					Render::ImageInfo {
									{ reinterpret_cast<char*>(data),
									  static_cast<size_t>(width * height * 4) },
						static_cast<uint32_t>(width),
						static_cast<uint32_t>(height),
				}
				);

				stbi_image_free(data);
			},
			[&](const fastgltf::sources::Array& vector) {
				int			   width, height, nrChannels;
				unsigned char* data = stbi_load_from_memory(
					reinterpret_cast<const stbi_uc*>(vector.bytes.data()),
					static_cast<int>(vector.bytes.size()),
					&width,
					&height,
					&nrChannels,
					4
				);
				texture = texture_from_image(
					ctx,
					Render::ImageInfo {
						{ reinterpret_cast<char*>(data),
						  static_cast<size_t>(width * height * 4) },
						static_cast<uint32_t>(width),
						static_cast<uint32_t>(height),
				}
				);
				stbi_image_free(data);
			},
			[&](const fastgltf::sources::BufferView& view) {
				auto& bufferView = asset.bufferViews[view.bufferViewIndex];
				auto& buffer	 = asset.buffers[bufferView.bufferIndex];
				// Yes, we've already loaded every buffer into some GL buffer. However, with
				// GL it's simpler to just copy the buffer data again for the texture.
				// Besides, this is just an example.
				std::visit(
					fastgltf::visitor {
						// We only care about VectorWithMime here, because we specify
						// LoadExternalBuffers, meaning
						// all buffers are already loaded into a vector.
						[&](const fastgltf::sources::Array& vector) {
							int			   width, height, nrChannels;
							unsigned char* data = stbi_load_from_memory(
								reinterpret_cast<const stbi_uc*>(
									vector.bytes.data() + bufferView.byteOffset
								),
								static_cast<int>(bufferView.byteLength),
								&width,
								&height,
								&nrChannels,
								4
							);
							texture = texture_from_image(
								ctx,
								Render::ImageInfo {
									.data	= { reinterpret_cast<char*>(data),
												//   std::strlen(reinterpret_cast<char*>(data))
												static_cast<size_t>(width * height * 4) },
									.width	= static_cast<uint32_t>(width),
									.height = static_cast<uint32_t>(height),
							}
							);
							stbi_image_free(data);
						},
						[](auto&& arg) { panic("unexpected image buffer view!"); },
					},
					buffer.data
				);
			},
			[](auto&& arg) { panic("unexpected image type!"); },
		},
		image.data
	);
	return texture;
}
}  // namespace dvdbr3o::Render
