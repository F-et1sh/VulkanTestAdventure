#pragma once
#include "FrameData.h"
#include "Window.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "GPUResourceManager.h"
#include "PipelineManager.h"

namespace VKTest {
	class Renderer {
	public:
		Renderer(Window* window) : p_Window{ window }, m_DeviceManager{ &m_SwapchainManager }, m_SwapchainManager{ &m_DeviceManager, window }, m_GPUResourceManager{ &m_DeviceManager }, m_PipelineManager{ &m_DeviceManager } {
			
			m_DeviceManager.CreateInstance();
			m_DeviceManager.SetupDebugMessenger();

			m_SwapchainManager.CreateSurface();

			m_DeviceManager.PickPhysicalDevice();
			m_DeviceManager.CreateLogicalDevice();

			m_SwapchainManager.CreateSwapchain();
			m_SwapchainManager.CreateImageViews();

			this->CreateRenderPass();
			m_GPUResourceManager.CreateDescriptorSetLayout();
			m_PipelineManager.CreateGraphicsPipeline();

			m_DeviceManager.CreateCommandPool();
			this->CreateColorResources();
			this->CreateDepthResources();

			/*
			
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
		void CreateRenderPass();
		void CreateColorResources();
		void CreateDepthResources();

	private:
		Window*								 p_Window = nullptr;
											 
		DeviceManager						 m_DeviceManager;
		SwapchainManager					 m_SwapchainManager;
		GPUResourceManager					 m_GPUResourceManager;
		PipelineManager						 m_PipelineManager;
											 
		vk::raii::RenderPass				 m_RenderPass = VK_NULL_HANDLE;

		vk::SampleCountFlagBits				 m_MSAA_Samples = vk::SampleCountFlagBits::e1; // TODO : configure sampling
											 
		std::vector<vk::raii::Framebuffer>	 m_Framebuffers;
		std::vector<FrameData>				 m_Frames;
	};
}