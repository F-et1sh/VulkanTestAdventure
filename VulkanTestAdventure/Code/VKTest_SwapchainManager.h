#pragma once

namespace VKTest {
    /* forward declaration */
    class DeviceManager;
    class Window;
    class RenderPassManager;

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   present_modes;

        SwapChainSupportDetails()  = default;
        ~SwapChainSupportDetails() = default;
    };

    class SwapchainManager {
    public:
        SwapchainManager(DeviceManager* device_manager, Window* window, RenderPassManager* render_pass_manager)
            : p_DeviceManager{ device_manager }, p_Window{ window }, p_RenderPassManager{ render_pass_manager } {}
        ~SwapchainManager() { this->Release(); }

        void Release();

        void CreateSurface();
        void CreateSwapchain();
        void CreateImageViews();
        void CreateColorResources();
        void CreateDepthResources();
        void CreateFramebuffers();

        VkSwapchainKHR                    getSwapchain() const noexcept { return m_Swapchain; }
        VkSurfaceKHR                      getSurface() const noexcept { return m_Surface; }
        VkFormat                          getImageFormat() const noexcept { return m_SwapchainImageFormat; }
        VkExtent2D                        getExtent() const noexcept { return m_SwapchainExtent; }
        const std::vector<VkFramebuffer>& getFramebuffers() const noexcept { return m_SwapchainFramebuffers; }

    public:
        static VkSurfaceFormatKHR      chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);
        static VkPresentModeKHR        chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
        static VkExtent2D              chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
        static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

        void recreateSwapchain();

    private:
        DeviceManager*     p_DeviceManager     = nullptr;
        Window*            p_Window            = nullptr;
        RenderPassManager* p_RenderPassManager = nullptr;

        VkSwapchainKHR m_Swapchain{};
        VkSurfaceKHR   m_Surface{};

        std::vector<VkImage> m_SwapchainImages;

        VkFormat   m_SwapchainImageFormat;
        VkExtent2D m_SwapchainExtent{};

        std::vector<VkImageView>   m_SwapchainImageViews;
        std::vector<VkFramebuffer> m_SwapchainFramebuffers;

        VkImage        m_ColorImage{};
        VkDeviceMemory m_ColorImageMemory{};
        VkImageView    m_ColorImageView{};

        VkImage        m_DepthImage{};
        VkDeviceMemory m_DepthImageMemory{};
        VkImageView    m_DepthImageView{};
    };
} // namespace VKTest
