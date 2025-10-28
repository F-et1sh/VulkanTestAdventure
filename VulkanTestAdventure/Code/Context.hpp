#pragma once
#include "Resources.hpp"

namespace vk_test {

    // Struct to hold an extension and its corresponding feature
    struct ExtensionInfo {
        const char* extension_name     = nullptr; // Name of the extension ex. VK_KHR_SWAPCHAIN_EXTENSION_NAME
        void*       feature            = nullptr; // [optional] Pointer to the feature structure for the extension
        bool        required           = true;    // [optional] If the extension is required
        uint32_t    spec_version       = 0;       // [optional] Spec version of the extension, this version or higher
        bool        exact_spec_version = false;   // [optional] If true, the spec version must match exactly
    };

    // Specific to the creation of Vulkan context
    // instanceExtensions   : Instance extensions: VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
    // deviceExtensions     : Device extensions: {{VK_KHR_SWAPCHAIN_EXTENSION_NAME}, {VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, &accelFeature}, {OTHER}}
    // queues               : All desired queues
    // instanceCreateInfoExt: Instance create info extension (ex: VkLayerSettingsCreateInfoEXT)
    // applicationName      : Application name
    // apiVersion           : Vulkan API version
    // alloc                : Allocation callbacks
    // enableAllFeatures    : If true, pull all capability of `features` from the physical device
    // forceGPU             : If != -1, use GPU index, useful to select a specific GPU
    struct ContextInitInfo {
        std::vector<const char*>   instance_extensions;
        std::vector<ExtensionInfo> device_extensions;
        std::vector<VkQueueFlags>  queues                   = { VK_QUEUE_GRAPHICS_BIT };
        void*                      instance_create_info_ext = nullptr;
        const char*                application_name         = "No Engine";
        uint32_t                   api_version              = VK_API_VERSION_1_4;
        VkAllocationCallbacks*     alloc                    = nullptr;
        bool                       enable_all_features      = true;
        int32_t                    force_gpu                = -1;
#if NDEBUG
        bool enable_validation_layers = false; // Disable validation layers in release
#else
        bool enable_validation_layers = true; // Enable validation layers
#endif
    };

    class Context {
    public:
        Context() = default;
        ~Context() { assert(m_Instance == VK_NULL_HANDLE); } // Forgot to call deinit ?

        void                   Release();
        [[nodiscard]] VkResult Initialize(const ContextInitInfo& context_init_info);

        [[nodiscard]] VkInstance                    getInstance() const { return m_Instance; }
        [[nodiscard]] VkDevice                      getDevice() const { return m_Device; }
        [[nodiscard]] VkPhysicalDevice              getPhysicalDevice() const { return m_PhysicalDevice; }
        [[nodiscard]] const QueueInfo&              getQueueInfo(uint32_t index) const { return m_QueueInfos[index]; }
        [[nodiscard]] const std::vector<QueueInfo>& getQueueInfos() const { return m_QueueInfos; }

    private:
        // Those functions are used internally to create the Vulkan context, but could be used externally if needed.
        [[nodiscard]] VkResult createInstance();
        [[nodiscard]] VkResult selectPhysicalDevice();
        [[nodiscard]] VkResult createDevice();
        [[nodiscard]] bool     findQueueFamilies();

        static VkResult getDeviceExtensions(VkPhysicalDevice physical_device, std::vector<VkExtensionProperties>& extension_properties);

    private:
        ContextInitInfo m_ContextInfo{}; // What was used to create the information

        VkInstance       m_Instance{};
        VkDevice         m_Device{};
        VkPhysicalDevice m_PhysicalDevice{};

        // For device creation
        VkPhysicalDeviceFeatures2        m_DeviceFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceVulkan11Features m_DeviceFeatures11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        VkPhysicalDeviceVulkan12Features m_DeviceFeatures12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        VkPhysicalDeviceVulkan13Features m_DeviceFeatures13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        VkPhysicalDeviceVulkan14Features m_DeviceFeatures14{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };

        // For Queue creation
        std::vector<VkQueueFlags>            m_DesiredQueues;
        std::vector<VkDeviceQueueCreateInfo> m_QueueCreateInfos;
        std::vector<QueueInfo>               m_QueueInfos;
        std::vector<std::vector<float>>      m_QueuePriorities; // Store priorities here

        // Callback for debug messages
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

        // Filters available Vulkan extensions based on desired extensions and their specifications.
        static bool filterAvailableExtensions(const std::vector<VkExtensionProperties>& available_extensions,
                                              const std::vector<ExtensionInfo>&         desired_extensions,
                                              std::vector<ExtensionInfo>&               filtered_extensions);
    };

    //--------------------------------------------------------------------------
    // This function adds the surface extensions needed for the platform.
    // If `deviceExtensions` is provided, then it also adds the
    // swapchain device extensions.
    void addSurfaceExtensions(std::vector<const char*>& instance_extensions, std::vector<ExtensionInfo>* device_extensions = nullptr);

} // namespace vk_test
