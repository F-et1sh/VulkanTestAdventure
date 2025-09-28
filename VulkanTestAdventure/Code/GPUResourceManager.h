#pragma once
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "RenderPassManager.h"
#include "Image.h"

namespace VKTest {
	class GPUResourceManager {
	public:
		GPUResourceManager(DeviceManager* device_manager, SwapchainManager* swapchain_manager, RenderPassManager* render_pass_manager);
		~GPUResourceManager() = default;

		void CreateDescriptorSetLayout();
		void CreateColorResources();
		void CreateDepthResources();

	public:
		inline vk::raii::DescriptorSetLayout& getDescriptorSetLayout()noexcept { return m_DescriptorSetLayout; }

	private:
		DeviceManager* p_DeviceManager = nullptr;
		SwapchainManager* p_SwapchainManager = nullptr;
		RenderPassManager* p_RenderPassManager = nullptr;

		vk::raii::DescriptorSetLayout		 m_DescriptorSetLayout = VK_NULL_HANDLE;
		vk::raii::DescriptorPool			 m_DescriptorPool = VK_NULL_HANDLE;
		std::vector<vk::raii::DescriptorSet> m_DescriptorSets;

		Image								 m_ColorImage;
		Image								 m_DepthImage;
	};
}