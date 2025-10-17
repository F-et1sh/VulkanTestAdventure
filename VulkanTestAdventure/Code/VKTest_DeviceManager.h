#pragma once
#include "VKTest_QueueFamilyIndices.h"

namespace VKTest {
#ifdef NDEBUG
    constexpr inline static bool ENABLE_VALIDATION_LAYERS = false;
#else
    constexpr inline static bool ENABLE_VALIDATION_LAYERS = true;
#endif

    constexpr inline static std::array VALIDATION_LAYERS{
        "VK_LAYER_KHRONOS_validation"
    };

    constexpr inline static std::array DEVICE_EXTENSIONS{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    /* forward declarations */
    class SwapchainManager;
    class Window;
    class RenderPassManager;
    class PipelineManager;
    class RenderMesh;

    class DeviceManager {
    public:
        DeviceManager(
            SwapchainManager*  swapchain_manager,
            Window*            window,
            RenderPassManager* render_pass_manager,
            PipelineManager*   pipeline_manager,
            RenderMesh*        render_mesh) :

                                       p_SwapchainManager{ swapchain_manager },
                                       p_Window{ window },
                                       p_RenderPassManager{ render_pass_manager },
                                       p_PipelineManager{ pipeline_manager },
                                       p_RenderMesh{ render_mesh } {}

        ~DeviceManager() { this->Release(); }

        void Release();

        void CreateInstance();
        void SetupDebugMessenger();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();

        void DrawFrame();

        VkInstance            getInstance() const noexcept { return m_Instance; }
        VkDevice              getDevice() const noexcept { return m_Device; }
        VkPhysicalDevice      getPhysicalDevice() const noexcept { return m_PhysicalDevice; }
        VkSampleCountFlagBits getMSAASamples() const noexcept { return m_MSAA_Samples; }

    public:
        static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
        void                      createImage(uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& image_memory);
        VkImageView               createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);
        VkFormat                  findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat                  findDepthFormat();
        VkCommandBuffer           beginSingleTimeCommands();
        void                      endSingleTimeCommands(VkCommandBuffer command_buffer);
        void                      copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
        void                      createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
        void                      transitionImageLayout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);
        void                      copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    private:
        static std::vector<const char*> getRequiredExtensions();
        static void                     populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
        static bool                     checkValidationLayerSupport();
        VkSampleCountFlagBits           getMaxUsableSampleCount();
        bool                            isDeviceSuitable(VkPhysicalDevice device);
        static bool                     checkDeviceExtensionSupport(VkPhysicalDevice device);
        uint32_t                        findMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);
        void                            recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index);
        void                            updateUniformBuffer(uint32_t current_image);

        static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger);
        static void     DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator);

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
            VK_TEST_SAY("Validation layer : " << callback_data->pMessage);
            return VK_FALSE;
        }

    private:
        SwapchainManager*  p_SwapchainManager  = nullptr;
        Window*            p_Window            = nullptr;
        RenderPassManager* p_RenderPassManager = nullptr;
        PipelineManager*   p_PipelineManager   = nullptr;
        RenderMesh*        p_RenderMesh        = nullptr;

        VkInstance m_Instance{};

        VkDevice         m_Device{};
        VkPhysicalDevice m_PhysicalDevice{};

        VkSampleCountFlagBits m_MSAA_Samples = VK_SAMPLE_COUNT_1_BIT;

        VkQueue m_GraphicsQueue{};
        VkQueue m_PresentQueue{};

        VkCommandPool                m_CommandPool{};
        std::vector<VkCommandBuffer> m_CommandBuffers;

        VkDebugUtilsMessengerEXT m_DebugMessenger{};

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence>     m_InFlightFences;
        uint32_t                 m_CurrentFrame = 0;
    };
} // namespace VKTest
