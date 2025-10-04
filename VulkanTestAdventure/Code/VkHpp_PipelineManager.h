#pragma once
#include "Vertices.h"
#include "VkHpp_GPUResourceManager.h"
#include "VkHpp_DeviceManager.h"
#include "VkHpp_RenderPassManager.h"

namespace VKHppTest {
	class PipelineManager {
	public:
		PipelineManager(DeviceManager* device_manager, RenderPassManager* render_pass_manager, GPUResourceManager* gpu_resource_manager, SwapchainManager* swapchain_manager);
		~PipelineManager() = default;

		void CreateGraphicsPipeline();
		void CreateDescriptorPool();

	public:
		void recordCommandBuffer(vk::raii::CommandBuffer& command_buffer, uint32_t image_index);

	private:
		static std::vector<char> readFile(const std::filesystem::path& path);
		vk::raii::ShaderModule createShaderModule(const std::vector<char>& code)const;

	private:
		DeviceManager* p_DeviceManager = nullptr;
		RenderPassManager* p_RenderPassManager = nullptr;
		GPUResourceManager* p_GPUResourceManager = nullptr;
		SwapchainManager* p_SwapchainManager = nullptr;

		vk::raii::PipelineLayout m_PipelineLayout   = VK_NULL_HANDLE;
		vk::raii::Pipeline		 m_GraphicsPipeline = VK_NULL_HANDLE;
	};
}