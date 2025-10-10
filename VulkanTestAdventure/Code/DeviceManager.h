#pragma once
#include "QueueFamilyIndices.h"

namespace VKTest {
#ifdef NDEBUG
    constexpr inline static bool ENABLE_VALIDATION_LAYERS = false;
#else
    constexpr inline static bool ENABLE_VALIDATION_LAYERS = true;
#endif

    constexpr inline static std::array VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation",
        "VK_EXT_debug_utils"
    };

    class SwapchainManager; // forward declaration

    class DeviceManager {
    public:
        DeviceManager(SwapchainManager* swapchain_manager) : p_SwapchainManager{ swapchain_manager } {}
        ~DeviceManager() { this->Release(); }

        void Release();

        void CreateInstance();
        void SetupDebugMessenger();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateCommandPool();
        void CreateCommandBuffers();

        VkInstance getInstance()const noexcept { return m_Instance; }

    private:
        static std::vector<const char*> getRequiredExtensions();
        static void                     populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
        static bool                     checkValidationLayerSupport();
        VkSampleCountFlagBits           getMaxUsableSampleCount();
        static bool                     isDeviceSuitable(VkPhysicalDevice device);
        static QueueFamilyIndices       findQueueFamilies(VkPhysicalDevice device);

        static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* create_info, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debug_messenger);
        static void     DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* allocator);

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
            VKTEST_SAY("Validation layer : " << callback_data->pMessage);
            return VK_FALSE;
        }

    private:
        SwapchainManager* p_SwapchainManager = nullptr;

        VkInstance m_Instance{};

        VkDevice         m_Device{};
        VkPhysicalDevice m_PhysicalDevice{};

        VkSampleCountFlagBits m_MSAA_Samples = VK_SAMPLE_COUNT_1_BIT;

        VkQueue m_GraphicsQueue{};
        VkQueue m_PresentQueue{};

        VkDebugUtilsMessengerEXT m_DebugMessenger{};
    };
} // namespace VKTest
