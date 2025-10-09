#pragma once

namespace vk_test {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;

        inline bool is_complete() const noexcept { return graphics_family.has_value() && present_family.has_value(); }
    };

#ifdef NDEBUG
    constexpr inline static bool ENABLE_VALIDATION_LAYERS = false;
#else
    constexpr inline static bool ENABLE_VALIDATION_LAYERS = true;
#endif

    constexpr inline static std::array VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation",
        "VK_EXT_debug_utils"
    };

    class DeviceManager {
    public:
        DeviceManager()  = default;
        ~DeviceManager() = default;

        void CreateInstance();
        void SetupDebugMessenger();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateCommandPool();
        void CreateCommandBuffers();

    private:
        std::vector<const char*> get_required_extensions();
        void                     populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);
        bool                     check_validation_layer_support();
        VkSampleCountFlagBits    getMaxUsableSampleCount();
        bool                     isDeviceSuitable(VkPhysicalDevice device);
        QueueFamilyIndices       findQueueFamilies(VkPhysicalDevice device);

        static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
        static void     DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data) {
            VKTEST_SAY("Validation layer : " << p_callback_data->pMessage);
            return VK_FALSE;
        }

    private:
        VkInstance instance;

        VkDevice         m_Device;
        VkPhysicalDevice m_PhysicalDevice;

        VkSampleCountFlagBits m_MSAA_Samples = VK_SAMPLE_COUNT_1_BIT;

        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;

        VkDebugUtilsMessengerEXT m_DebugMessenger;
    };
} // namespace vk_test
