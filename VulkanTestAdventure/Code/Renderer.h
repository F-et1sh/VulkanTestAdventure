#pragma once
#include "Vertices.h"
#include "FrameData.h"
#include "Window.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "GPUResourceManager.h"

namespace VKTest {
	class Renderer {
	public:
		Renderer(Window* window) : p_Window{ window }, m_DeviceManager{ &m_SwapchainManager }, m_SwapchainManager{ &m_DeviceManager, window }, m_GPUResourceManager{} {
			
			m_DeviceManager.CreateInstance();
			m_DeviceManager.SetupDebugMessenger();

			m_SwapchainManager.CreateSurface();

			m_DeviceManager.PickPhysicalDevice();
			m_DeviceManager.CreateLogicalDevice();

			m_SwapchainManager.CreateSwapchain();
			m_SwapchainManager.CreateImageViews();

			this->CreateRenderPass();
			//createDescriptorSetLayout();
			this->CreateGraphicsPipeline();

			m_DeviceManager.CreateCommandPool();

			/*
			
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
		void CreateRenderPass();
		void CreateGraphicsPipeline();

	private:
		static std::vector<char> readFile(const std::filesystem::path& path);
		vk::raii::ShaderModule createShaderModule(const std::vector<char>& code)const;

	private:
		Window*								 p_Window = nullptr;
											 
		DeviceManager						 m_DeviceManager;
		SwapchainManager					 m_SwapchainManager;
		GPUResourceManager					 m_GPUResourceManager;
											 
		vk::raii::RenderPass				 m_RenderPass = VK_NULL_HANDLE;
		vk::raii::PipelineLayout			 m_PipelineLayout = VK_NULL_HANDLE;
		vk::raii::Pipeline					 m_GraphicsPipeline = VK_NULL_HANDLE;
											 
		vk::raii::DescriptorSetLayout		 m_DescriptorSetLayout = VK_NULL_HANDLE; // TODO : Transfer this to the GPUResourceManager
		vk::raii::DescriptorPool			 m_DescriptorPool = VK_NULL_HANDLE;		 // TODO : Transfer this to the GPUResourceManager
		std::vector<vk::raii::DescriptorSet> m_DescriptorSets;						 // TODO : Transfer this to the GPUResourceManager

		vk::SampleCountFlagBits				 m_MSAA_Samples = vk::SampleCountFlagBits::e1; // TODO : configure sampling
											 
		std::vector<vk::raii::Framebuffer>	 m_Framebuffers;
		std::vector<FrameData>				 m_Frames;
	};
}