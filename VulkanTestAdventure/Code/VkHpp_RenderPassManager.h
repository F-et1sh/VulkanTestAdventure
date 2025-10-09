#pragma once
#include "VkHpp_SwapchainManager.h"

namespace VKHppTest {
    class RenderPassManager {
    public:
        RenderPassManager(DeviceManager* device_manager, SwapchainManager* swapchain_manager);
        ~RenderPassManager() = default;

        void CreateRenderPass();

        inline vk::SampleCountFlagBits getMSAASamples() const noexcept { return m_MSAA_Samples; }
        inline vk::raii::RenderPass&   getRenderPass() noexcept { return m_RenderPass; }

    private:
        DeviceManager*    p_DeviceManager    = nullptr;
        SwapchainManager* p_SwapchainManager = nullptr;

        vk::raii::RenderPass    m_RenderPass   = VK_NULL_HANDLE;
        vk::SampleCountFlagBits m_MSAA_Samples = vk::SampleCountFlagBits::e1;
    };
} // namespace VKHppTest
