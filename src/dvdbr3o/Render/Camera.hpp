#pragma once

#include "dvdbr3o/Render/Context.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <webgpu/webgpu_cpp.h>

#include <algorithm>

namespace dvdbr3o::Render {

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
		rotation				   = glm::normalize(glm::angleAxis(delta_yaw, world_up) * rotation);

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

struct CameraBindgroup {
	struct Data {
		glm::mat4 view;
		glm::mat4 proj;
	};

	wgpu::Buffer	buffer;
	wgpu::BindGroup bindgroup;

	CameraBindgroup(const Context& ctx = Context::global()) {
		const wgpu::BufferDescriptor buf_desc {
			.usage			  = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
			.size			  = sizeof(Data),
			.mappedAtCreation = false,
		};
		buffer = ctx.device.CreateBuffer(&buf_desc);
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
