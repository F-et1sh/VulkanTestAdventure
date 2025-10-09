#pragma once

namespace vk_test {
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
        static auto get_required_extensions() -> std::vector<const char*>;
        static void                     populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info);
        static auto                     check_validation_layer_support() -> bool;

    
        static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data);

    
        VkInstance m_Instance;

        VkPhysicalDevice m_PhysicalDevice;
        VkDevice         m_Device;

        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;

        VkDebugUtilsMessengerEXT m_DebugMessenger;
    };
} // namespace VKTest
