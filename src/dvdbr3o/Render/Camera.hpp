#pragma once

#include "dvdbr3o/Render/Bindgroup.hpp"
#include "dvdbr3o/Render/Context.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <webgpu/webgpu_cpp.h>

#include <algorithm>
#include <array>
#include <cstdint>

namespace dvdbr3o::Render {

struct Viewport {
	int32_t width  = 0;
	int32_t height = 0;
};

struct Global {
	float	 time	   = 0.0f;
	float	 delta_time = 0.0f;
	float	 frame	   = 0.0f;
	Viewport viewport {};
};

struct Camera {
	inline static constexpr float	  mouse_turn_speed	= glm::pi<float>();
	inline static constexpr float	  max_pitch_radians = glm::radians(89.0f);
	inline static constexpr glm::vec3 world_up { 0.0f, 1.0f, 0.0f };
	inline static constexpr glm::vec3 local_right { 1.0f, 0.0f, 0.0f };
	inline static constexpr glm::vec3 local_forward { 0.0f, 0.0f, -1.0f };

	glm::vec3						  pos { 0.0f, 0.0f, 3.0f };
	glm::quat						  rotation { 1.0f, 0.0f, 0.0f, 0.0f };
	float							  fov	 = glm::radians(60.0f);
	float							  aspect = 16.0f / 9.0f;
	float							  z_near = 0.1f;
	float							  z_far	 = 1000.0f;

	[[nodiscard]] auto				  forward() const -> glm::vec3 {
		return glm::normalize(rotation * local_forward);
	}

	[[nodiscard]] auto right() const -> glm::vec3 { return glm::normalize(rotation * local_right); }

	[[nodiscard]] auto up() const -> glm::vec3 { return glm::normalize(rotation * world_up); }

	[[nodiscard]] auto mat_view() const -> glm::mat4 {
		return glm::lookAtRH(pos, pos + forward(), up());
	}

	[[nodiscard]] auto mat_proj() const -> glm::mat4 {
		return glm::perspectiveRH_ZO(fov, aspect, z_near, z_far);
	}

	auto update_aspect(int width, int height) -> void {
		aspect = static_cast<float>(std::max(width, 1)) / static_cast<float>(std::max(height, 1));
	}

	auto rotate(float delta_yaw, float delta_pitch) -> void {
		rotation = glm::normalize(glm::angleAxis(delta_yaw, world_up) * rotation);

		const auto current_forward = forward();
		const auto current_pitch   = std::asin(std::clamp(current_forward.y, -1.0f, 1.0f));
		const auto target_pitch =
			std::clamp(current_pitch + delta_pitch, -max_pitch_radians, max_pitch_radians);
		const auto clamped_pitch_delta = target_pitch - current_pitch;

		rotation = glm::normalize(glm::angleAxis(clamped_pitch_delta, right()) * rotation);
	}

	auto rotate_from_mouse(float dx, float dy, int width, int height) -> void {
		const auto scale_denominator = static_cast<float>(std::max(std::min(width, height), 1));
		const auto normalized_dx	 = dx / scale_denominator;
		const auto normalized_dy	 = dy / scale_denominator;
		rotate(normalized_dx * mouse_turn_speed, -normalized_dy * mouse_turn_speed);
	}
};

struct GlobalHandle {
	struct Data {
		float	time;
		float	delta_time;
		float	frame;
		float	_padding0;
		int32_t viewport_width;
		int32_t viewport_height;
		int32_t _padding1;
		int32_t _padding2;
	};

	wgpu::Buffer	buffer;
	wgpu::BindGroup bindgroup;

	inline static auto from(
		const wgpu::BindGroupLayout& layout, const Context& ctx = Context::global()
	) -> GlobalHandle {
		GlobalHandle out;
		const wgpu::BufferDescriptor buf_desc {
			.usage			  = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
			.size			  = sizeof(Data),
			.mappedAtCreation = false,
		};
		out.buffer	  = ctx.device.CreateBuffer(&buf_desc);
		out.bindgroup = bindgroup_from(
			ctx,
			layout,
			std::to_array<wgpu::BindGroupEntry>({
				{
					.binding = 0,
					.buffer	 = out.buffer,
					.size	 = sizeof(Data),
				},
			})
		);
		return out;
	}

	auto update(const Context& ctx, const Global& global) const -> void {
		const Data data {
			.time			 = global.time,
			.delta_time		 = global.delta_time,
			.frame			 = global.frame,
			._padding0		 = 0.0f,
			.viewport_width	 = global.viewport.width,
			.viewport_height = global.viewport.height,
			._padding1		 = 0,
			._padding2		 = 0,
		};
		ctx.queue.WriteBuffer(buffer, 0, &data, sizeof(Data));
	}

	auto update(const Global& global) const -> void { update(Context::global(), global); }
};

struct CameraHandle {
	struct Data {
		glm::mat4 view;
		glm::mat4 proj;
	};

	wgpu::Buffer	buffer;
	wgpu::BindGroup bindgroup;

	inline static auto from(
		const wgpu::BindGroupLayout& layout, const Context& ctx = Context::global()
	) -> CameraHandle {
		CameraHandle out;
		const wgpu::BufferDescriptor buf_desc {
			.usage			  = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
			.size			  = sizeof(Data),
			.mappedAtCreation = false,
		};
		out.buffer	  = ctx.device.CreateBuffer(&buf_desc);
		out.bindgroup = bindgroup_from(
			ctx,
			layout,
			std::to_array<wgpu::BindGroupEntry>({
				{
					.binding = 0,
					.buffer	 = out.buffer,
					.size	 = sizeof(Data),
				},
			})
		);
		return out;
	}

	auto update(const Context& ctx, const Camera& camera) const -> void {
		const Data data {
			.view = camera.mat_view(),
			.proj = camera.mat_proj(),
		};
		ctx.queue.WriteBuffer(buffer, 0, &data, sizeof(Data));
	}

	auto update(const Camera& camera) const -> void { update(Context::global(), camera); }
};

}  // namespace dvdbr3o::Render
