#pragma once
#include "Resources.hpp"
#include "barriers.hpp"

namespace vk_test {
    class Swapchain {
    public:
        Swapchain() = default;
        ~Swapchain() { assert(m_Swapchain == VK_NULL_HANDLE && "Missing deinit()"); }

        struct InitInfo {
            VkPhysicalDevice physical_device{};
            VkDevice         device{};
            QueueInfo        queue{};
            VkSurfaceKHR     surface{};
            VkCommandPool    command_pool{};
            VkPresentModeKHR preferred_vsync_off_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            VkPresentModeKHR preferred_vsync_on_mode  = VK_PRESENT_MODE_FIFO_KHR;
        };

        void Release();
        // Initialize the swapchain with the provided context and surface, then we can create and re-create it
        VkResult Initialize(const InitInfo& info);

        /*--
         * Create the swapchain using the provided context, surface, and vSync option. The actual window size is returned.
         * Queries the GPU capabilities, selects the best surface format and present mode, and creates the swapchain accordingly.
        -*/
        VkResult InitializeResources(VkExtent2D& out_window_size, bool v_sync = true);

        /*--
         * Recreate the swapchain, typically after a window resize or when it becomes invalid.
         * This waits for all rendering to be finished before destroying the old swapchain and creating a new one.
        -*/
        VkResult ReinitializeResources(VkExtent2D& out_window_size, bool v_sync = true);

        /*--
         * Destroy the swapchain and its associated resources.
         * This function is also called when the swapchain needs to be recreated.
        -*/
        void ReleaseResources();

        void requestRebuild() { m_NeedRebuild = true; }
        bool needRebuilding() const { return m_NeedRebuild; }

        VkImage     getImage() const { return m_Images[m_FrameImageIndex].image; }
        VkImageView getImageView() const { return m_Images[m_FrameImageIndex].image_view; }
        VkFormat    getImageFormat() const { return m_ImageFormat; }
        uint32_t    getMaxFramesInFlight() const { return m_MaxFramesInFlight; }

        VkSemaphore getImageAvailableSemaphore() const { return m_FrameResources[m_FrameResourceIndex].image_available_semaphore; }
        VkSemaphore getRenderFinishedSemaphore() const { return m_FrameResources[m_FrameResourceIndex].render_finished_semaphore; }

    private:
        // Represents an image within the swapchain that can be rendered to.
        struct Image {
            VkImage     image{};      // Image to render to
            VkImageView image_view{}; // Image view to access the image
        };
        /*--
         * Resources associated with each frame being processed.
         * Each frame has its own set of resources, mainly synchronization primitives
        -*/
        struct FrameResources {
            VkSemaphore image_available_semaphore{}; // Signals when the image is ready for rendering
            VkSemaphore render_finished_semaphore{}; // Signals when rendering is finished
        };

        // We choose the format that is the most common, and that is supported by* the physical device.
        static VkSurfaceFormat2KHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormat2KHR>& available_formats);

        /*--
         * The present mode is chosen based on the vSync option
         * The `preferredVsyncOnMode` is used when vSync is enabled and the mode is supported.
         * The `preferredVsyncOffMode` is used when vSync is disabled and the mode is supported.
         * Otherwise, from most preferred to least:
         *   1. IMMEDIATE mode, when vSync is disabled (tearing allowed), since it's lowest-latency.
         *   2. MAILBOX mode, since it's the lowest-latency mode without tearing. Note that frame pacing is needed
                when vSync is on.
         *   3. FIFO mode, since all swapchains must support it.
        -*/
        VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes, bool v_sync = true);

    private:
        Swapchain(Swapchain&)                  = delete;  //
        Swapchain& operator=(const Swapchain&) = default; // Allow copy only for internal use in deinit() to restore pristine state

        VkPhysicalDevice m_PhysicalDevice{}; // The physical device (GPU)
        VkDevice         m_Device{};         // The logical device (interface to the physical device)
        QueueInfo        m_Queue{};          // The queue used to submit command buffers to the GPU
        VkSwapchainKHR   m_Swapchain{};      // The swapchain
        VkFormat         m_ImageFormat{};    // The format of the swapchain images
        VkSurfaceKHR     m_Surface{};        // The surface to present images to
        VkCommandPool    m_CommandPool{};    // The command pool for the swapchain

        std::vector<Image>          m_Images;                     // The swapchain images and their views
        std::vector<FrameResources> m_FrameResources;             // Synchronization primitives for each frame
        uint32_t                    m_FrameResourceIndex = 0;     // Current frame index, cycles through frame resources
        uint32_t                    m_FrameImageIndex    = 0;     // Index of the swapchain image we're currently rendering to
        bool                        m_NeedRebuild        = false; // Flag indicating if the swapchain needs to be rebuilt

        VkPresentModeKHR m_PreferredVsyncOffMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // used if available
        VkPresentModeKHR m_PreferredVsyncOnMode  = VK_PRESENT_MODE_FIFO_KHR;      // used if available

        // Triple buffering allows us to pipeline CPU and GPU work, which gives us
        // good throughput if their sum takes more than a frame.
        // But if we're using VK_PRESENT_MODE_FIFO_KHR without frame pacing and
        // workloads are < 1 frame, then work can be waiting for multiple frames for
        // the swapchain image to be available, increasing latency. For this reason,
        // it's good to use a frame pacer with the swapchain.
        uint32_t m_MaxFramesInFlight = 3;
    };
} // namespace vk_test
