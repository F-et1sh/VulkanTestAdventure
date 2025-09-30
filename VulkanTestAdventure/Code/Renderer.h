#pragma once
#include "Window.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "GPUResourceManager.h"
#include "PipelineManager.h"

namespace VKTest {
	class Renderer {
	public:
		Renderer(Window* window) : 
			p_Window{ window }, 
			m_DeviceManager{ &m_SwapchainManager }, 
			m_SwapchainManager{ &m_DeviceManager, window, &m_GPUResourceManager, &m_RenderPassManager }, 
			m_GPUResourceManager{ &m_DeviceManager, &m_SwapchainManager, &m_RenderPassManager },
			m_PipelineManager{ &m_DeviceManager, &m_RenderPassManager, &m_GPUResourceManager, &m_SwapchainManager },
			m_RenderPassManager{ &m_DeviceManager, &m_SwapchainManager } {
			
			m_DeviceManager.CreateInstance();
			m_DeviceManager.SetupDebugMessenger();

			m_SwapchainManager.CreateSurface();

			m_DeviceManager.PickPhysicalDevice();
			m_DeviceManager.CreateLogicalDevice();

			m_SwapchainManager.CreateSwapchain();
			m_SwapchainManager.CreateImageViews();

			m_RenderPassManager.CreateRenderPass();
			m_GPUResourceManager.CreateDescriptorSetLayout();
			m_PipelineManager.CreateGraphicsPipeline();

			m_DeviceManager.CreateCommandPool();
			m_GPUResourceManager.CreateColorResources();
			m_GPUResourceManager.CreateDepthResources();

			m_SwapchainManager.CreateFramebuffers();

			/*
			
			createTextureImage();
			createTextureImageView();
			createTextureSampler();
			loadModel();
			
			*/

			m_GPUResourceManager.CreateVertexBuffer();
			m_GPUResourceManager.CreateIndexBuffer();

			m_GPUResourceManager.CreateUniformBuffers();
			m_GPUResourceManager.CreateDescriptorPool();
			m_GPUResourceManager.CreateDescriptorSets();

			m_DeviceManager.CreateCommandBuffers();
			m_GPUResourceManager.CreateSyncObjects();
		}
		~Renderer() = default;

		void DrawFrame();

		inline Window* getWindow()noexcept { return p_Window; }

		inline DeviceManager& getDeviceManager()noexcept { return m_DeviceManager; }
		inline SwapchainManager& getSwapchainManager()noexcept { return m_SwapchainManager; }
		inline GPUResourceManager& getGPUResourceManager()noexcept { return m_GPUResourceManager; }

	private:
		Window*								 p_Window = nullptr;
											 
		DeviceManager						 m_DeviceManager;
		SwapchainManager					 m_SwapchainManager;
		GPUResourceManager					 m_GPUResourceManager;
		PipelineManager						 m_PipelineManager;
		RenderPassManager					 m_RenderPassManager;

		uint32_t							 m_CurrentFrame = 0;
	};
}