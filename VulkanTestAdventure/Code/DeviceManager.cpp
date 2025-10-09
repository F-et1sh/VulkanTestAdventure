#include "pch.h"
#include "DeviceManager.h"

void vk_test::DeviceManager::CreateInstance() {
    if (ENABLE_VALIDATION_LAYERS && !check_validation_layer_support()) {
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

    auto extensions                     = get_required_extensions();
    create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    if (ENABLE_VALIDATION_LAYERS) {
        create_info.enabledLayerCount   = static_cast<uint32_t>(VALIDATION_LAYERS.size());
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();

        populate_debug_messenger_create_info(debug_create_info);
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

void vk_test::DeviceManager::SetupDebugMessenger() {
}

void vk_test::DeviceManager::PickPhysicalDevice() {
}

void vk_test::DeviceManager::CreateLogicalDevice() {
}

void vk_test::DeviceManager::CreateCommandPool() {
}

void vk_test::DeviceManager::CreateCommandBuffers() {
}

std::vector<const char*> vk_test::DeviceManager::get_required_extensions() {
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

void vk_test::DeviceManager::populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
    create_info                 = {};
    create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
}

bool vk_test::DeviceManager::check_validation_layer_support() {
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
