#include "pch.h"
#include "DeviceManager.h"

void VKTest::DeviceManager::CreateInstance() {
    if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport()) {
        VKTEST_RUNTIME_ERROR("ERROR : Validation layers requested, but not available");
    }

    VkApplicationInfo app_info{};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "No Engine";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo create_info{};
    create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    auto extensions                     = getRequiredExtensions();
    create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    if (ENABLE_VALIDATION_LAYERS) {
        create_info.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();

        populateDebugMessengerCreateInfo(debug_create_info);
        create_info.pNext = (&debug_create_info);
    }
    else {
        create_info.enabledLayerCount = 0;
        create_info.pNext             = nullptr;
    }

    if (vkCreateInstance(&create_info, nullptr, &m_Instance) != VK_SUCCESS) {
        VKTEST_RUNTIME_ERROR("ERROR : Failed to create instance");
    }
}

void VKTest::DeviceManager::SetupDebugMessenger() {
    if (!ENABLE_VALIDATION_LAYERS) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT create_info;
    populateDebugMessengerCreateInfo(create_info);

    if (VKTest::DeviceManager::CreateDebugUtilsMessengerEXT(m_Instance, &create_info, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VKTest::DeviceManager::PickPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_Instance, &device_count, nullptr);

    if (device_count == 0) {
        VKTEST_RUNTIME_ERROR("ERROR : Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(m_Instance, &device_count, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            m_PhysicalDevice = device;
            m_MSAA_Samples   = getMaxUsableSampleCount();
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void VKTest::DeviceManager::CreateLogicalDevice() {
}

void VKTest::DeviceManager::CreateCommandPool() {
}

void VKTest::DeviceManager::CreateCommandBuffers() {
}

std::vector<const char*> VKTest::DeviceManager::getRequiredExtensions() {
    uint32_t     glfw_extension_count = 0;
    const char** glfw_extensions      = nullptr;
    glfw_extensions                   = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (ENABLE_VALIDATION_LAYERS) {
        for (const auto& l : VALIDATION_LAYERS) {
            extensions.emplace_back(l);
        }
    }

    return extensions;
}

void VKTest::DeviceManager::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
    create_info                 = {};
    create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
}

bool VKTest::DeviceManager::checkValidationLayerSupport() {
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char* layer_name : VALIDATION_LAYERS) {
        bool layer_found = false;

        for (const auto& layer_properties : available_layers) {
            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            return false;
        }
    }

    return true;
}

VkSampleCountFlagBits VKTest::DeviceManager::getMaxUsableSampleCount() {
    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physical_device_properties);

    VkSampleCountFlags counts = physical_device_properties.limits.framebufferColorSampleCounts & physical_device_properties.limits.framebufferDepthSampleCounts;
    if ((counts & VK_SAMPLE_COUNT_64_BIT) != 0U) {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if ((counts & VK_SAMPLE_COUNT_32_BIT) != 0U) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if ((counts & VK_SAMPLE_COUNT_16_BIT) != 0U) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if ((counts & VK_SAMPLE_COUNT_8_BIT) != 0U) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if ((counts & VK_SAMPLE_COUNT_4_BIT) != 0U) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if ((counts & VK_SAMPLE_COUNT_2_BIT) != 0U) {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

bool VKTest::DeviceManager::isDeviceSuitable(VkPhysicalDevice device) {
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensions_supported = checkDeviceExtensionSupport(device);

    bool swap_chain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swap_chain_support = querySwapChainSupport(device);
        swap_chain_adequate                        = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(device, &supported_features);

    return indices.is_complete() && extensions_supported && swap_chain_adequate && (supported_features.samplerAnisotropy != 0U);
}

QueueFamilyIndices VKTest::DeviceManager::findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int i = 0;
    for (const auto& queue_family : queue_families) {
        if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
            indices.graphics_family = i;
        }

        VkBool32 present_support = 0U;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

        if (present_support != 0U) {
            indices.present_family = i;
        }

        if (indices.is_complete()) {
            break;
        }

        i++;
    }

    return indices;
}

VkResult VKTest::DeviceManager::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void VKTest::DeviceManager::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* p_allocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debug_messenger, p_allocator);
    }
}
