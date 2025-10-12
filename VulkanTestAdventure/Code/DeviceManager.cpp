#include "pch.h"
#include "DeviceManager.h"

void VKTest::DeviceManager::Release() {
}

void VKTest::DeviceManager::CreateInstance() {
    if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport()) {
        VK_TEST_RUNTIME_ERROR("ERROR : Validation layers requested, but not available");
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
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to create instance");
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
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to find GPUs with Vulkan support");
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

    if (m_PhysicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void VKTest::DeviceManager::CreateLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(p_SwapchainManager->getSurface());

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t>                   unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

    float queue_priority = 1.0F;
    for (uint32_t queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount       = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};
    device_features.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos    = queue_create_infos.data();

    create_info.pEnabledFeatures = &device_features;

    create_info.enabledExtensionCount   = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

    if (ENABLE_VALIDATION_LAYERS) {
        create_info.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    }
    else {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(m_PhysicalDevice, &create_info, nullptr, &m_Device) != VK_SUCCESS) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to create logical device");
    }

    vkGetDeviceQueue(m_Device, indices.graphics_family.value(), 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, indices.present_family.value(), 0, &m_PresentQueue);
}

void VKTest::DeviceManager::CreateCommandPool() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(p_SwapchainManager->getSurface());

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphics_family.value();

    if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to create graphics command pool");
    }
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
    create_info.pfnUserCallback = DebugCallback;
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
    QueueFamilyIndices indices = findQueueFamilies(p_SwapchainManager->getSurface());

    bool extensions_supported = checkDeviceExtensionSupport();

    bool swap_chain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swap_chain_support = VKTest::SwapchainManager::querySwapChainSupport(m_PhysicalDevice, p_SwapchainManager->getSurface());
        swap_chain_adequate                        = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
    }

    VkPhysicalDeviceFeatures supported_features;
    vkGetPhysicalDeviceFeatures(device, &supported_features);

    return indices.is_complete() && extensions_supported && swap_chain_adequate && (supported_features.samplerAnisotropy != 0U);
}

bool VKTest::DeviceManager::checkDeviceExtensionSupport() {
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

uint32_t VKTest::DeviceManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    VK_TEST_RUNTIME_ERROR("ERROR : Failed to find suitable memory type");
}

QueueFamilyIndices VKTest::DeviceManager::findQueueFamilies(VkSurfaceKHR surface) {
    QueueFamilyIndices indices{};

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queue_family_count, queue_families.data());

    int i = 0;
    for (const auto& queue_family : queue_families) {
        if ((queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0U) {
            indices.graphics_family = i;
        }

        VkBool32 present_support = 0U;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, surface, &present_support);

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

void VKTest::DeviceManager::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width  = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth  = 1;
    imageInfo.mipLevels     = mipLevels;
    imageInfo.arrayLayers   = 1;
    imageInfo.format        = format;
    imageInfo.tiling        = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage         = usage;
    imageInfo.samples       = numSamples;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_Device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to create image");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to allocate image memory");
    }

    vkBindImageMemory(m_Device, image, imageMemory, 0);
}

VkImageView VKTest::DeviceManager::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels) {
    VkImageViewCreateInfo view_info{};
    view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image                           = image;
    view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format                          = format;
    view_info.subresourceRange.aspectMask     = aspect_flags;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;

    VkImageView image_view = nullptr;
    if (vkCreateImageView(m_Device, &view_info, nullptr, &image_view) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image view!");
    }

    return image_view;
}

VkFormat VKTest::DeviceManager::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    VK_TEST_RUNTIME_ERROR("ERROR : Failed to find supported format");
}

VkFormat VKTest::DeviceManager::findDepthFormat() {
    return findSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
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
