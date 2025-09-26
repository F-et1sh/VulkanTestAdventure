#include "pch.h"
#include "DeviceManager.h"

#include "SwapchainManager.h"

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

#ifdef _DEBUG
    vk::DebugUtilsMessengerCreateInfoEXT create_info;
    this->configureDebugMessengerCreateInfo(create_info);

    m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(create_info);
#else
    // ..
#endif // _DEBUG
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

    auto& surface = p_SwapchainManager->getSurface();

    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queue_family_properties = m_PhysicalDevice.getQueueFamilyProperties();

    // get the first index into queue_family_properties which supports graphics
    auto lambda = [](auto const& qfp) { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); };
    auto graphics_queue_family_property = std::ranges::find_if(queue_family_properties, lambda);

    auto graphics_index = static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_family_property));

    // determine a queue_family_index that supports present
    // first check if the graphics_index is good enough
    bool is_supports_present = m_PhysicalDevice.getSurfaceSupportKHR(graphics_index, *surface);
    auto present_index = is_supports_present ? graphics_index : static_cast<uint32_t>(queue_family_properties.size());

    if (present_index == queue_family_properties.size()) {
        // the graphics_index doesn't support present -> look for another family index that supports both graphics and present
        for (size_t i = 0; i < queue_family_properties.size(); i++) {

            bool is_supports_present = m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface);
            if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) && is_supports_present) {

                graphics_index = static_cast<uint32_t>(i);
                present_index = graphics_index;
                break;
            }
        }

        if (present_index == queue_family_properties.size()) {
            // there's nothing like a single family index that supports both graphics and present -> look for another family index that supports present
            for (size_t i = 0; i < queue_family_properties.size(); i++) {

                bool is_supports_present = m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface);
                
                if (is_supports_present) {
                    present_index = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }

    if ((graphics_index == queue_family_properties.size()) || (present_index == queue_family_properties.size()))
        RUNTIME_ERROR("ERROR : Could not find a queue for graphics or present");

    // query for Vulkan 1.3 features
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features;
    extended_dynamic_state_features.extendedDynamicState = vk::True;
    
    vk::PhysicalDeviceVulkan13Features vulkan13_features;
    vulkan13_features.dynamicRendering = vk::True;
    vulkan13_features.pNext = &extended_dynamic_state_features;
    
    auto features = m_PhysicalDevice.getFeatures2();
    features.pNext = &vulkan13_features;

    // create a device
    float                     queue_priority = 0.0f;
    vk::DeviceQueueCreateInfo device_queue_create_info{
        vk::DeviceQueueCreateFlags{},   // Flags
        graphics_index,                 // Queue Family Index
        1,                              // Queue Count
        &queue_priority                 // Priorities
    };

    vk::DeviceCreateInfo device_create_info{};
    device_create_info.setEnabledExtensionCount(REQUIRED_DEVICE_EXTENSION.size());
    device_create_info.setPpEnabledExtensionNames(REQUIRED_DEVICE_EXTENSION.data());

    device_create_info.setQueueCreateInfoCount(1);
    device_create_info.setPQueueCreateInfos(&device_queue_create_info);

    device_create_info.setPNext(&features);

    m_QueueFamilyIndices.graphics_family = graphics_index;
    m_QueueFamilyIndices.present_family = present_index;

    m_Device = vk::raii::Device{ m_PhysicalDevice, device_create_info };
    m_GraphicsQueue = vk::raii::Queue{ m_Device, graphics_index, 0 };
    m_PresentQueue = vk::raii::Queue{ m_Device, present_index, 0 };
}

void VKTest::DeviceManager::CreateCommandPool() {
    uint32_t queue_family_indices = findQueueFamilies(m_PhysicalDevice, vk::QueueFlagBits::eGraphics);

    vk::CommandPoolCreateInfo pool_info{
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, // Flags
        queue_family_indices                                // Queue Family Index
    };

    m_CommandPool = m_Device.createCommandPool(pool_info);
}

void VKTest::DeviceManager::configureDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& create_info)const noexcept {
    create_info.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    create_info.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
    create_info.setPfnUserCallback(DebugCallback);
}

std::vector<const char*> VKTest::DeviceManager::getRequiredExtensions()const {
    uint32_t glfw_extension_count = 0;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (ENABLE_VALIDATION_LAYERS) extensions.push_back(vk::EXTDebugUtilsExtensionName);

    return extensions;
}

vk::raii::ImageView VKTest::DeviceManager::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags, uint32_t mip_levels) const {
    vk::ImageViewCreateInfo create_info{
                vk::ImageViewCreateFlags{}, // Flags
                image,						// Image
                vk::ImageViewType::e2D,		// Image View Type
                format,						// Format
                {},							// Components
                vk::ImageSubresourceRange{
                    aspect_flags,			// Aspect Mask
                    0,						// Base Mip Level
                    mip_levels,				// Level Count
                    0,						// Base Array Layer
                    1,						// Layer Count
                }
    };

    return vk::raii::ImageView{ m_Device, create_info };
}

uint32_t VKTest::DeviceManager::findQueueFamilies(vk::PhysicalDevice device, vk::QueueFlagBits flags)const {
    // find the index of the first queue family that supports graphics
    std::vector<vk::QueueFamilyProperties> queue_family_properties = device.getQueueFamilyProperties();

    auto lambda = [flags](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & flags; };

    // get the first index into queue_family_properties which supports graphics
    auto queue_family_property = std::find_if(queue_family_properties.begin(), queue_family_properties.end(), lambda);

    return static_cast<uint32_t>(std::distance(queue_family_properties.begin(), queue_family_property));
}