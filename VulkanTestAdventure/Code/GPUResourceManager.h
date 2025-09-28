#pragma once
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "Image.h"

namespace VKTest {
	class GPUResourceManager {
	public:
		GPUResourceManager(DeviceManager* device_manager, SwapchainManager* swapchain_manager);
		~GPUResourceManager() = default;

		void CreateDescriptorSetLayout();

	private:
		void CreateColorResources();
		void CreateDepthResources();

	private:
		DeviceManager* p_DeviceManager = nullptr;
		SwapchainManager* p_SwapchainManager = nullptr;

		vk::raii::DescriptorSetLayout		 m_DescriptorSetLayout = VK_NULL_HANDLE;
		vk::raii::DescriptorPool			 m_DescriptorPool = VK_NULL_HANDLE;
		std::vector<vk::raii::DescriptorSet> m_DescriptorSets;

		Image								 m_ColorImage;
		Image								 m_DepthImage;
	};
}