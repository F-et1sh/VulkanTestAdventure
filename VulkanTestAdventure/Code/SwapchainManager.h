#pragma once

namespace VKTest {
    /* forward declaration */
    class DeviceManager;
    class Window;

    class SwapchainManager {
    public:
        SwapchainManager(DeviceManager* device_manager, Window* window) : p_DeviceManager{ device_manager }, p_Window{ window } {}
        ~SwapchainManager() { this->Release(); }

        void Release();

        void CreateSurface();

        VkSwapchainKHR getSwapchain() const noexcept { return m_Swapchain; }

    private:
        DeviceManager* p_DeviceManager = nullptr;
        Window*        p_Window        = nullptr;

        VkSwapchainKHR m_Swapchain;
        VkSurfaceKHR   m_Surface;

        std::vector<VkImage> m_SwapchainImages;

        VkFormat   m_SwapchainImageFormat;
        VkExtent2D m_SwapchainExtent;

        std::vector<VkImageView>   m_SwapchainImageViews;
        std::vector<VkFramebuffer> m_SwapchainFramebuffers;
    };
} // namespace VKTest
