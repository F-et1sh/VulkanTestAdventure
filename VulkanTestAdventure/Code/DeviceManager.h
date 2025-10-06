#pragma once

namespace VKTest {
#ifdef NDEBUG
    constexpr inline static bool ENABLE_VALIDATION_LAYERS = false;
#else
    constexpr inline static bool ENABLE_VALIDATION_LAYERS = true;
#endif

    constexpr inline static std::array<const char*, 1> VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };

    class DeviceManager {
    public:
        DeviceManager()  = default;
        ~DeviceManager() = default;

    private:
        void CreateInstance();

    private:
        static bool checkValidationLayerSupport();

    private:
        VkInstance m_Instance;

        VkPhysicalDevice m_PhysicalDevice;
        VkDevice         m_Device;

        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;

        VkDebugUtilsMessengerEXT m_DebugMessenger;
    };
} // namespace VKTest
