#pragma once

namespace VKTest {
    /* forward declaration */
    class DeviceManager;
    class SwapchainManager;

    class RenderPassManager {
    public:
        RenderPassManager(DeviceManager* device_manager, SwapchainManager* swapchain_manager) : p_DeviceManager{ device_manager }, p_SwapchainManager{ swapchain_manager } {}
        ~RenderPassManager() = default;

        void CreateRenderPass();

        VkRenderPass getRenderPass() const noexcept { return m_RenderPass; }

    private:
        VkFormat findDepthFormat();

    private:
        DeviceManager*    p_DeviceManager    = nullptr;
        SwapchainManager* p_SwapchainManager = nullptr;

        VkRenderPass m_RenderPass{};
    };
} // namespace VKTest
