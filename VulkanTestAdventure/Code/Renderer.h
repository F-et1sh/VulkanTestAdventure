#pragma once
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "GPUResourceManager.h"

namespace VKTest {
	class Renderer {
	public:
		Renderer() = default;
		~Renderer() = default;

		inline DeviceManager& getDeviceManager()noexcept { return m_DeviceManager; }
		inline SwapchainManager& getSwapchainManager()noexcept { return m_SwapchainManager; }
		inline GPUResourceManager& getGPUResourceManager()noexcept { return m_GPUResourceManager; }

	private:
		DeviceManager		m_DeviceManager{ this };
		SwapchainManager	m_SwapchainManager{ this };
		GPUResourceManager	m_GPUResourceManager{ this };
	};
}