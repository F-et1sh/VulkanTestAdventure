#pragma once
#include "IRenderer.hpp"

#include "resources.hpp"

namespace vk_test {
    // Renderer creation info
    struct RendererVulkanCreateInfo {
        // Vulkan
        VkInstance                   instance{ VK_NULL_HANDLE };        // Vulkan instance
        VkDevice                     device{ VK_NULL_HANDLE };          // Logical device
        VkPhysicalDevice             physical_device{ VK_NULL_HANDLE }; // Physical device
        std::vector<nvvk::QueueInfo> queues;                            // Queue family and properties (0: Graphics)
        uint32_t                     texture_pool_size = 128U;          // Maximum number of textures in the descriptor pool

        // Swapchain
        // VK_PRESENT_MODE_MAX_ENUM_KHR means no preference
        VkPresentModeKHR preferred_vsync_off_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;
        VkPresentModeKHR preferred_vsync_on_mode  = VK_PRESENT_MODE_MAX_ENUM_KHR;
    };

    class RendererVulkan : public IRenderer {
    public:
        RendererVulkan(Window* window) : p_Window{ window } {}
        ~RendererVulkan() override { this->Release(); }

        void Release();
        void Initialize(RendererVulkanCreateInfo& info);

        void run();

        inline uint32_t getFrameCycleIndex() const { return m_FrameRingCurrent; }
        inline uint32_t getFrameCycleSize() const { return uint32_t(m_FrameData.size()); }

    private:
        void createTransientCommandPool();
        void createDescriptorPool();
        void createFrameSubmission(uint32_t num_frames);
        void resetFreeQueue(uint32_t size);

    private:
        Window* p_Window{ nullptr };

        // Vulkan resources
        VkInstance                   m_Instance{ VK_NULL_HANDLE };
        VkPhysicalDevice             m_PhysicalDevice{ VK_NULL_HANDLE };
        VkDevice                     m_Device{ VK_NULL_HANDLE };
        std::vector<nvvk::QueueInfo> m_Queues;                // All queues, first one should be a graphics queue
        VkSurfaceKHR                 m_Surface{};             // The window surface
        VkCommandPool                m_TransientCmdPool{};    // The command pool
        VkDescriptorPool             m_DescriptorPool{};      // Renderer descriptor pool
        uint32_t                     m_MaxTexturePool{ 128 }; // Maximum number of textures in the descriptor pool

        // Frame resources and synchronization (Swapchain, Command buffers, Semaphores, Fences)
        //nvvk::Swapchain m_swapchain;
        struct FrameData {
            VkCommandPool   command_pool{};   // Command pool for recording commands for this frame
            VkCommandBuffer command_buffer{}; // Command buffer containing the frame's rendering commands
            uint64_t        frame_number{};   // Timeline value for synchronization (increases each frame)
        };
        std::vector<FrameData> m_FrameData;                // Collection of per-frame resources to support multiple frames in flight
        VkSemaphore            m_FrameTimelineSemaphore{}; // Timeline semaphore used to synchronize CPU submission with GPU completion
        uint32_t               m_FrameRingCurrent{ 0 };    // Current frame index in the ring buffer (cycles through available frames) : static for resource free queue

        // Fine control over the frame submission
        std::vector<VkSemaphoreSubmitInfo>     m_WaitSemaphores;   // Possible extra frame wait semaphores
        std::vector<VkSemaphoreSubmitInfo>     m_SignalSemaphores; // Possible extra frame signal semaphores
        std::vector<VkCommandBufferSubmitInfo> m_CommandBuffers;   // Possible extra frame command buffers

        VkExtent2D m_ViewportSize{ 0, 0 }; // Size of the viewport
        VkExtent2D m_WindowSize{ 0, 0 };   // Size of the window

        // temp
        std::vector<std::vector<std::function<void()>>> m_ResourceFreeQueue; // Queue of functions to free resources
    };
} // namespace vk_test
