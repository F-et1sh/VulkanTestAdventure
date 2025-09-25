#include "pch.h"
#include "DeviceManager.h"

void VKTest::DeviceManager::CreateInstance() {
    constexpr static vk::ApplicationInfo app_info{
        "VKTest",                   // Application Name
        vk::makeVersion(1, 0, 0),   // Application Version
        nullptr,                    // Engine Name
        vk::makeVersion(1, 0, 0),   // Engine Version
        vk::ApiVersion14            // API Version
    };

    std::vector<char const*> required_layers;
    if (ENABLE_VALIDATION_LAYERS) {
        required_layers.assign(VALIDATION_LAYERS.begin(), VALIDATION_LAYERS.end());
    }

    // check if the required layers are supported by the vulkan implementation
    auto layer_properties = m_Context.enumerateInstanceLayerProperties();
    for (auto const& required_layer : required_layers) {

        auto lambda = [required_layer](auto const& layerProperty) { return strcmp(layerProperty.layerName, required_layer) == 0; };
        if (std::ranges::none_of(layer_properties, lambda))
            RUNTIME_ERROR("Required layer not supported : " + std::string(required_layer));
    }

    // get the required extensions
    auto required_extensions = getRequiredExtensions();

    // check if the required extensions are supported by the vulkan implementation
    auto extension_properties = m_Context.enumerateInstanceExtensionProperties();
    for (auto const& required_extension : required_extensions) {

        auto lambda = [required_extension](auto const& extensionProperty) { return strcmp(extensionProperty.extensionName, required_extension) == 0; };
        if (std::ranges::none_of(extension_properties, lambda))
            RUNTIME_ERROR("Required extension not supported: " + std::string(required_extension));
    }

    vk::InstanceCreateInfo create_info{
        vk::InstanceCreateFlags{},                          // Flags
        &app_info,                                          // Application Info
        static_cast<uint32_t>(required_layers.size()),      // Enabled Layer Count
        required_layers.data(),                             // Enabled Layer Names
        static_cast<uint32_t>(required_extensions.size()),  // Enabled Extension Count
        required_extensions.data()                          // Enabled Extension Names
    };

    m_Instance = vk::raii::Instance{ m_Context, create_info };
}

void VKTest::DeviceManager::SetupDebugMessenger() {
    if (!ENABLE_VALIDATION_LAYERS) return;

    vk::DebugUtilsMessengerCreateInfoEXT create_info;
    this->configureDebugMessengerCreateInfo(create_info);

    m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(create_info);
}

void VKTest::DeviceManager::PickPhysicalDevice() {
    std::vector<vk::raii::PhysicalDevice> devices = m_Instance.enumeratePhysicalDevices();

    auto lambda = [&](auto const& device) {
            // check if the device supports the Vulkan 1.3 API version
            bool supports_vulkan_1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

            // check if any of the queue families support graphics operations
            auto queue_families = device.getQueueFamilyProperties();
            bool supports_graphics = std::ranges::any_of(queue_families, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

            // check if all required device extensions are available
            auto available_device_extensions = device.enumerateDeviceExtensionProperties();
            bool supports_all_required_extensions =
                std::ranges::all_of(REQUIRED_DEVICE_EXTENSION,
                    [&available_device_extensions](auto const& requiredDeviceExtension)
                    {
                        return std::ranges::any_of(available_device_extensions,
                            [requiredDeviceExtension](auto const& availableDeviceExtension)
                            { return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; });
                    });

            auto features = device.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
            bool supports_required_features = features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering && features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

            return supports_vulkan_1_3 && supports_graphics && supports_all_required_extensions && supports_required_features;
        };

    auto device_iterator = std::ranges::find_if(devices, lambda);

    if (device_iterator != devices.end()) m_PhysicalDevice = *device_iterator;
    else RUNTIME_ERROR("ERROR : Failed to find a suitable GPU");
}

void VKTest::DeviceManager::CreateLogicalDevice() {
    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queue_family_properties = m_PhysicalDevice.getQueueFamilyProperties();

    // get the first index into queue_family_properties which supports graphics
    auto graphics_queue_family_property = std::ranges::find_if(queue_family_properties, [](auto const& qfp)
        { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); });
    
    if (graphics_queue_family_property == queue_family_properties.end()) RUNTIME_ERROR("ERROR : No graphics queue family found");

    auto graphics_index = static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_family_property));

    // query for Vulkan 1.3 features
    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        feature_chain{
        {},
        {},
        {}
    };
    feature_chain.get<vk::PhysicalDeviceVulkan13Features>().setDynamicRendering(true);
    feature_chain.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().setExtendedDynamicState(true);

    // create a device
    float                     queue_priority = 0.0f;
    vk::DeviceQueueCreateInfo device_queue_create_info{
        vk::DeviceQueueCreateFlags{},   // Flags
        graphics_index,                 // Queue Family Index
        1,                              // Queue Count
        &queue_priority                 // Queue Priorities
    };
    
    vk::DeviceCreateInfo      device_create_info{
        vk::DeviceCreateFlags{},                                    // Flags
        1,                                                          // Queue Create Info Count
        &device_queue_create_info,                                  // Queue Create Infos
        {},
        {},
        static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSION.size()),    // Enabled Extension Count
        REQUIRED_DEVICE_EXTENSION.data(),                           // Enabled Extension Names
        {},
        &feature_chain.get<vk::PhysicalDeviceFeatures2>()           // Next
    };

    m_Device = vk::raii::Device{ m_PhysicalDevice, device_create_info };
    m_GraphicsQueue = vk::raii::Queue{ m_Device, graphics_index, 0 };
}

void VKTest::DeviceManager::CreateCommandPool() {

}

void VKTest::DeviceManager::configureDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& create_info)noexcept {
    create_info.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    create_info.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
    create_info.setPfnUserCallback(DebugCallback);
}

std::vector<const char*> VKTest::DeviceManager::getRequiredExtensions() {
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (ENABLE_VALIDATION_LAYERS) extensions.push_back(vk::EXTDebugUtilsExtensionName);

    return extensions;
}