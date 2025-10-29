#pragma once
#include "Window.hpp"
#include "Resources.hpp"
#include "Context.hpp"
#include "Swapchain.hpp"

namespace vk_test {
    constexpr inline static bool IS_VSYN_WANTED = true;

    // Forward declarations
    class Application;

    //-------------------------------------------------------------------------------------------------
    // Interface for application elements
    struct IAppElement {
        // Interface
        virtual void onAttach(Application* app) {}                            // Called once at start
        virtual void onDetach() {}                                            // Called before destroying the application
        virtual void onResize(VkCommandBuffer cmd, const VkExtent2D& size) {} // Called when the viewport size is changing
        virtual void onUIRender() {}                                          // Called for anything related to UI
        virtual void onUIMenu() {}                                            // This is the menubar to create
        virtual void onPreRender() {}                                         // called post onUIRender and prior onRender (looped over all elements)
        virtual void onRender(VkCommandBuffer cmd) {}                         // For anything to render within a frame
        virtual void onFileDrop(const std::filesystem::path& filename) {}     // For when a file is dragged on top of the window
        virtual void onLastHeadlessFrame() {};                                // Called at the end of the last frame in headless mode

        virtual ~IAppElement() = default;
    };

    struct ApplicationCreateInfo {
        // General
        std::string name{ "VulkanApp" }; // Application name

        // Vulkan
        VkInstance             instance{ VK_NULL_HANDLE };        // Vulkan instance
        VkDevice               device{ VK_NULL_HANDLE };          // Logical device
        VkPhysicalDevice       physical_device{ VK_NULL_HANDLE }; // Physical device
        std::vector<QueueInfo> queues;                            // Queue family and properties (0: Graphics)
        uint32_t               texture_pool_size = 128U;          // Maximum number of textures in the descriptor pool

        // GLFW
        glm::uvec2 window_size{ 0, 0 }; // Window size (width, height) or Viewport size (headless)
        bool       vsync{ true };       // Enable V-Sync by default

        // Headless
        bool     headless{ false };         // Run without a window
        uint32_t headless_frame_count{ 1 }; // Frames to render in headless mode

        // Swapchain
        // VK_PRESENT_MODE_MAX_ENUM_KHR means no preference
        VkPresentModeKHR preferred_vsync_off_mode = VK_PRESENT_MODE_MAX_ENUM_KHR;
        VkPresentModeKHR preferred_vsync_on_mode  = VK_PRESENT_MODE_MAX_ENUM_KHR;
    };

    class Application {
    public:
        Application(const glm::vec2& window_resolution, const std::string& window_title, int window_monitor)
            : m_Window{ window_resolution, window_title, window_monitor } {}
        ~Application() = default;

        void Release();
        void Initialize(ApplicationCreateInfo& info);
        void Loop();

        uint32_t getFrameCycleSize() const { return uint32_t(m_FrameData.size()); }

    private:
        void        testAndSetWindowSizeAndPos(const glm::uvec2& window_size);
        void        createDescriptorPool();
        void        createTransientCommandPool();
        void        createFrameSubmission(uint32_t num_frames);
        void        resetFreeQueue(uint32_t size);
        static bool isWindowPosValid(const glm::ivec2& window_position);

    private:
        Window m_Window;

        // Vulkan resources
        VkInstance             m_Instance{ VK_NULL_HANDLE };
        VkPhysicalDevice       m_PhysicalDevice{ VK_NULL_HANDLE };
        VkDevice               m_Device{ VK_NULL_HANDLE };
        std::vector<QueueInfo> m_Queues;                 // All queues, first one should be a graphics queue
        VkSurfaceKHR           m_Surface{};              // The window surface
        VkCommandPool          m_TransientCommandPool{}; // The command pool
        VkDescriptorPool       m_DescriptorPool{};       // Application descriptor pool
        uint32_t               m_MaxTexturePool{ 128 };  // Maximum number of textures in the descriptor pool

        // Frame resources and synchronization (Swapchain, Command buffers, Semaphores, Fences)
        Swapchain m_Swapchain;
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

        std::vector<std::vector<std::function<void()>>> m_ResourceFreeQueue; // Queue of functions to free resources

        VkExtent2D m_ViewportExtent{ 0, 0 }; // Size of the viewport
        VkExtent2D m_WindowExtent{ 0, 0 };   // Size of the window

        glm::ivec2 m_WindowPosition{};
        glm::uvec2 m_WindowSize{};
    };
} // namespace vk_test
