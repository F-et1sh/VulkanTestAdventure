#pragma once
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "GPUResourceManager.h"

namespace VKTest {
	class Renderer {
	public:
		Renderer() = default;
		~Renderer() = default;

	private:
		DeviceManager		m_DeviceManager{ this };
		SwapchainManager	m_SwapchainManager{ this };
		GPUResourceManager	m_GPUResourceManager{ this };
	};
}