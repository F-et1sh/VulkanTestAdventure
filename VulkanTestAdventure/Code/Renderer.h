#pragma once
#include "Window.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "GPUResourceManager.h"

namespace VKTest {
	class Renderer {
	public:
		Renderer(Window* p_window) : p_Window{ p_window }, m_DeviceManager{}, m_SwapchainManager{ &m_DeviceManager, p_window }, m_GPUResourceManager{} {}
		~Renderer() = default;

		inline Window* getWindow()noexcept { return p_Window; }

		inline DeviceManager& getDeviceManager()noexcept { return m_DeviceManager; }
		inline SwapchainManager& getSwapchainManager()noexcept { return m_SwapchainManager; }
		inline GPUResourceManager& getGPUResourceManager()noexcept { return m_GPUResourceManager; }

	private:
		Window* p_Window = nullptr;

		DeviceManager		m_DeviceManager;
		SwapchainManager	m_SwapchainManager;
		GPUResourceManager	m_GPUResourceManager;
	};
}