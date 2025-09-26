#pragma once
#include "Window.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "GPUResourceManager.h"

namespace VKTest {
	class Renderer {
	public:
		Renderer(Window* p_window) : p_Window{ p_window }, m_DeviceManager{ &m_SwapchainManager }, m_SwapchainManager{ &m_DeviceManager, p_window }, m_GPUResourceManager{} {
			
			m_DeviceManager.CreateInstance();
			m_DeviceManager.SetupDebugMessenger();

			m_SwapchainManager.CreateSurface();

			m_DeviceManager.PickPhysicalDevice();
			m_DeviceManager.CreateLogicalDevice();
			m_DeviceManager.CreateCommandPool();

			m_SwapchainManager.CreateSwapchain();
			m_SwapchainManager.CreateImageViews();

			/*
			
			createInstance();
			setupDebugMessenger();
			createSurface();
			pickPhysicalDevice();
			createLogicalDevice();
			createSwapChain();
			createImageViews();
			createRenderPass();
			createDescriptorSetLayout();
			createGraphicsPipeline();
			createCommandPool();
			createColorResources();
			createDepthResources();
			createFramebuffers();
			createTextureImage();
			createTextureImageView();
			createTextureSampler();
			loadModel();
			createVertexBuffer();
			createIndexBuffer();
			createUniformBuffers();
			createDescriptorPool();
			createDescriptorSets();
			createCommandBuffers();
			createSyncObjects();

			*/
		}
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