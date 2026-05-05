#pragma once

#include "dvdbr3o/Utils/Misc.hpp"

#include <webgpu/webgpu_cpp.h>
#include <spdlog/spdlog.h>

#include <array>
#include <limits>

namespace dvdbr3o::Render {
struct Context {
	wgpu::Instance instance;
	wgpu::Adapter  adapter;
	wgpu::Device   device;
	wgpu::Queue	   queue;

public:
	Context() :
		instance([]() {
			const auto					   feats = std::to_array<wgpu::InstanceFeatureName>({
				wgpu::InstanceFeatureName::TimedWaitAny,
			});
			const wgpu::InstanceDescriptor desc	 = {
				 .requiredFeatureCount = feats.size(),
				 .requiredFeatures	   = feats.data(),
			};
			return wgpu::CreateInstance(&desc);
		}()) {
		const wgpu::RequestAdapterOptions adapter_opt = {};

		instance.WaitAny(
			instance.RequestAdapter(
				&adapter_opt,
				wgpu::CallbackMode::WaitAnyOnly,
				[this](
					wgpu::RequestAdapterStatus status,
					const wgpu::Adapter&	   adapter,
					wgpu::StringView		   message
				) {
					if (status == wgpu::RequestAdapterStatus::Success) {
						this->adapter					   = adapter;

						wgpu::DeviceDescriptor device_desc = {};
						device_desc.SetUncapturedErrorCallback([](const wgpu::Device&,
																  wgpu::ErrorType  errorType,
																  wgpu::StringView message) {
							spdlog::error(
								"[WGPU/{}]: {}",
								(int)errorType,
								std::string_view(message)
							);
						});
						instance.WaitAny(
							adapter.RequestDevice(
								&device_desc,
								wgpu::CallbackMode::WaitAnyOnly,
								[this](
									wgpu::RequestDeviceStatus status,
									const wgpu::Device&		  device,
									wgpu::StringView		  message
								) {
									if (status == wgpu::RequestDeviceStatus::Success) {
										this->device = device;
										this->queue	 = device.GetQueue();
									} else
										spdlog::critical(
											"failed to request device: {}",
											message.data
										);
								}
							),
							std::numeric_limits<uint32_t>::max()
						);
					} else {
						spdlog::critical("failed to request adapter: {}", message.data);
					}
				}
			),
			std::numeric_limits<uint32_t>::max()
		);
	}

	inline static auto& global() {
		static Context context;
		return context;
	}
};

inline auto& global_context() {
	return Context::global();
}
}  // namespace dvdbr3o::Render