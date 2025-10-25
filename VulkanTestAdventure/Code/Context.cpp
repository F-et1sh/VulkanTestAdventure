#include "pch.h"
#include "Context.hpp"

static VKAPI_ATTR VkBool32 VKAPI_CALL VkContextDebugReport(VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                                                           VkDebugUtilsMessageTypeFlagsEXT             message_type,
                                                           const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                           void*                                       user_data) {
    VK_TEST_SAY("VULKAN ERROR : " << callback_data->pMessage);
    return VK_FALSE;
}

void vk_test::Context::Release() {
    if (m_Device != nullptr) {
        vkDestroyDevice(m_Device, m_ContextInfo.alloc);
    }

    if (m_Instance != nullptr) {
        if ((m_DebugMessenger != nullptr) && (vkDestroyDebugUtilsMessengerEXT != nullptr)) {
            vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, m_ContextInfo.alloc);
            m_DebugMessenger = nullptr;
        }
        vkDestroyInstance(m_Instance, m_ContextInfo.alloc);
    }
    m_Device   = VK_NULL_HANDLE;
    m_Instance = VK_NULL_HANDLE;
}

VkResult vk_test::Context::Initialize(const ContextInitInfo& context_init_info) {
    m_ContextInfo = context_init_info;

    // Initialize the Vulkan loader
    volkInitialize();

    this->createInstance();
    this->selectPhysicalDevice();
    this->createDevice();

    return VK_SUCCESS;
}

VkResult vk_test::Context::createInstance() {
    VkApplicationInfo app_info{
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = m_ContextInfo.application_name,
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "My Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = m_ContextInfo.api_version,
    };

    std::vector<const char*> layers;
    if (m_ContextInfo.enable_validation_layers) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }

    VkInstanceCreateInfo create_info{
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = m_ContextInfo.instance_create_info_ext,
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = uint32_t(layers.size()),
        .ppEnabledLayerNames     = layers.data(),
        .enabledExtensionCount   = uint32_t(m_ContextInfo.instance_extensions.size()),
        .ppEnabledExtensionNames = m_ContextInfo.instance_extensions.data(),
    };

    VkResult result = vkCreateInstance(&create_info, m_ContextInfo.alloc, &m_Instance);
    if (result != VK_SUCCESS) {
        // Since the debug utils aren't available yet and this is usually the first
        // place an app can fail, we should print some additional help here.
        VK_TEST_SAY(
            "vkCreateInstance failed with error %s!\n"
            "You may need to install a newer Vulkan SDK, or check that it is properly installed.\n"
            << result);
        return result;
    }

    // Loading Vulkan functions
    volkLoadInstance(m_Instance);

    if (m_ContextInfo.enable_validation_layers) {
        if (vkCreateDebugUtilsMessengerEXT != nullptr) {
            VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
            debug_messenger_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT  // GPU info, bug
                                                          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; // Invalid usage
            debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT       // Violation of spec
                                                      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;   // Non-optimal use
            //      debug_messenger_create_info.messageSeverity |=
            //          VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
            //      debug_messenger_create_info.messageType |=
            //          VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT;
            debug_messenger_create_info.pfnUserCallback = VkContextDebugReport;
            vkCreateDebugUtilsMessengerEXT(m_Instance, &debug_messenger_create_info, nullptr, &m_DebugMessenger);
        }
        else {
            VK_TEST_SAY("Missing VK_EXT_DEBUG_UTILS extension, cannot use vkCreateDebugUtilsMessengerEXT for validation layers.\n");
        }
    }

    return VK_SUCCESS;
}

VkResult vk_test::Context::selectPhysicalDevice() {
    if (m_Instance == VK_NULL_HANDLE) {
        VK_TEST_SAY("%s: m_Instance was null; call createInstance() first");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(m_Instance, &device_count, nullptr);
    if (device_count == 0) {
        VK_TEST_SAY("%s: Failed to find any GPUs with Vulkan support!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    std::vector<VkPhysicalDevice> gpus(device_count);
    vkEnumeratePhysicalDevices(m_Instance, &device_count, gpus.data());

    // Find the discrete GPU if present, or use first one available.
    if ((m_ContextInfo.force_gpu == -1) || (m_ContextInfo.force_gpu >= int(device_count))) {
        m_PhysicalDevice = gpus[0];
        for (VkPhysicalDevice& device : gpus) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);
            if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                m_PhysicalDevice = device;
            }
        }
    }
    else {
        // Using specified GPU
        m_PhysicalDevice = gpus[m_ContextInfo.force_gpu];
    }

    { // Check for available Vulkan version
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
        uint32_t api_version = properties.apiVersion;
        if ((VK_VERSION_MAJOR(api_version) < VK_VERSION_MAJOR(m_ContextInfo.api_version)) ||
            (VK_VERSION_MINOR(api_version) < VK_VERSION_MINOR(m_ContextInfo.api_version))) {

            VK_TEST_SAY("Requested Vulkan version is higher than available version"
                        << "\nReqired : " << VK_VERSION_MAJOR(m_ContextInfo.api_version) << "." << VK_VERSION_MINOR(m_ContextInfo.api_version)
                        << "\nAvailable : " << VK_VERSION_MAJOR(api_version) << "." << VK_VERSION_MINOR(api_version));
            m_PhysicalDevice = {};
            return VK_ERROR_INITIALIZATION_FAILED;
        }
    }

    // Query the physical device features
    m_DeviceFeatures.pNext = &m_DeviceFeatures11;
    if (m_ContextInfo.api_version >= VK_API_VERSION_1_2) {
        m_DeviceFeatures11.pNext = &m_DeviceFeatures12;
    }
    if (m_ContextInfo.api_version >= VK_API_VERSION_1_3) {
        m_DeviceFeatures12.pNext = &m_DeviceFeatures13;
    }
    if (m_ContextInfo.api_version >= VK_API_VERSION_1_4) {
        m_DeviceFeatures13.pNext = &m_DeviceFeatures14;
    }
    vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &m_DeviceFeatures);

    // Find the queues that we need
    m_DesiredQueues = m_ContextInfo.queues;
    if (!findQueueFamilies()) {
        m_PhysicalDevice = {};
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    return VK_SUCCESS;
}

static inline void pNextChainPushFront(void* main_struct, void* new_struct) {
    auto* new_base_struct  = reinterpret_cast<VkBaseOutStructure*>(new_struct);
    auto* main_base_struct = reinterpret_cast<VkBaseOutStructure*>(main_struct);

    new_base_struct->pNext  = main_base_struct->pNext;
    main_base_struct->pNext = new_base_struct;
}

VkResult vk_test::Context::createDevice() {
    if (m_PhysicalDevice == VK_NULL_HANDLE) {
        VK_TEST_SAY("m_PhysicalDevice was null; call selectPhysicalDevice() first.");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Filter the available extensions otherwise the device creation will fail
    std::vector<ExtensionInfo>         filtered_extensions;
    std::vector<VkExtensionProperties> extension_properties;
    getDeviceExtensions(m_PhysicalDevice, extension_properties);
    bool all_found = filterAvailableExtensions(extension_properties, m_ContextInfo.device_extensions, filtered_extensions);
    if (!all_found) {
        m_PhysicalDevice = {};
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    m_ContextInfo.device_extensions = std::move(filtered_extensions);
    // nvutils::ScopedTimer st(__FUNCTION__);
    // Chain all custom features to the pNext chain of m_deviceFeatures
    for (const auto& extension : m_ContextInfo.device_extensions) {
        if (extension.feature != nullptr) {
            pNextChainPushFront(&m_DeviceFeatures, extension.feature);
        }
    }
    // Activate features on request
    if (m_ContextInfo.enable_all_features) {
        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &m_DeviceFeatures);
    }

    // List of extensions to enable
    std::vector<const char*> enabled_extensions;
    for (const auto& ext : m_ContextInfo.device_extensions) {
        enabled_extensions.push_back(ext.extension_name);
    }

    VkDeviceCreateInfo create_info{
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &m_DeviceFeatures,
        .queueCreateInfoCount    = uint32_t(m_QueueCreateInfos.size()),
        .pQueueCreateInfos       = m_QueueCreateInfos.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(enabled_extensions.size()),
        .ppEnabledExtensionNames = enabled_extensions.data(),
    };

    const VkResult result = vkCreateDevice(m_PhysicalDevice, &create_info, m_ContextInfo.alloc, &m_Device);
    if (VK_SUCCESS != result) {
        VK_TEST_SAY("ERROR : vkCreateDevice failed with error");
        return result;
    }
    volkLoadDevice(m_Device);

    for (auto& queue : m_QueueInfos) {
        vkGetDeviceQueue(m_Device, queue.familyIndex, queue.queueIndex, &queue.queue);
    }

    return VK_SUCCESS;
}

bool vk_test::Context::findQueueFamilies() {
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queue_family_count, queue_families.data());

    std::unordered_map<uint32_t, uint32_t> queue_family_usage;
    for (uint32_t i = 0; i < queue_family_count; ++i) {
        queue_family_usage[i] = 0;
    }

    for (size_t i = 0; i < m_DesiredQueues.size(); ++i) {
        bool found = false;
        for (uint32_t j = 0; j < queue_family_count; ++j) {
            // Check for an exact match and unused queue family
            // Avoid queue family with VK_QUEUE_GRAPHICS_BIT if not needed
            if ((queue_families[j].queueFlags & m_DesiredQueues[i]) == m_DesiredQueues[i] && queue_family_usage[j] == 0 && (((m_DesiredQueues[i] & VK_QUEUE_GRAPHICS_BIT) != 0u) || ((queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0u))) {
                m_QueueInfos.push_back({ j, queue_family_usage[j] });
                queue_family_usage[j]++;
                found = true;
                break;
            }
        }

        if (!found) {
            for (uint32_t j = 0; j < queue_family_count; ++j) {
                // Check for an exact match and allow reuse if queue count not exceeded
                // Avoid queue family with VK_QUEUE_GRAPHICS_BIT if not needed
                if ((queue_families[j].queueFlags & m_DesiredQueues[i]) == m_DesiredQueues[i] && queue_family_usage[j] < queue_families[j].queueCount && (((m_DesiredQueues[i] & VK_QUEUE_GRAPHICS_BIT) != 0u) || ((queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0u))) {
                    m_QueueInfos.push_back({ j, queue_family_usage[j] });
                    queue_family_usage[j]++;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            for (uint32_t j = 0; j < queue_family_count; ++j) {
                // Check for a partial match and allow reuse if queue count not exceeded
                // Avoid queue family with VK_QUEUE_GRAPHICS_BIT if not needed
                if (((queue_families[j].queueFlags & m_DesiredQueues[i]) != 0u) && queue_family_usage[j] < queue_families[j].queueCount && (((m_DesiredQueues[i] & VK_QUEUE_GRAPHICS_BIT) != 0u) || ((queue_families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0u))) {
                    m_QueueInfos.push_back({ j, queue_family_usage[j] });
                    queue_family_usage[j]++;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            for (uint32_t j = 0; j < queue_family_count; ++j) {
                // Check for a partial match and allow reuse if queue count not exceeded
                if (((queue_families[j].queueFlags & m_DesiredQueues[i]) != 0u) && queue_family_usage[j] < queue_families[j].queueCount) {
                    m_QueueInfos.push_back({ j, queue_family_usage[j] });
                    queue_family_usage[j]++;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            // If no suitable queue family is found, assert a failure
            VK_TEST_SAY("Failed to find a suitable queue family");
            return false;
        }
    }

    for (const auto& usage : queue_family_usage) {
        if (usage.second > 0) {
            m_QueuePriorities.emplace_back(usage.second, 1.0F); // Same priority for all queues in a family
            m_QueueCreateInfos.push_back({ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, nullptr, 0, usage.first, usage.second, m_QueuePriorities.back().data() });
        }
    }
    return true;
}

bool vk_test::Context::filterAvailableExtensions(const std::vector<VkExtensionProperties>& available_extensions,
                                                 const std::vector<ExtensionInfo>&         desired_extensions,
                                                 std::vector<ExtensionInfo>&               filtered_extensions) {
    bool all_found = true;

    // Create a map for quick lookup of available extensions and their versions
    std::unordered_map<std::string, uint32_t> available_extensions_map;
    for (const auto& ext : available_extensions) {
        available_extensions_map[ext.extensionName] = ext.specVersion;
    }

    // Iterate through all desired extensions
    for (const auto& desired_extension : desired_extensions) {
        auto it = available_extensions_map.find(desired_extension.extension_name);

        bool     found         = it != available_extensions_map.end();
        uint32_t spec_version  = found ? it->second : 0;
        bool     valid_version = desired_extension.exact_spec_version ? desired_extension.spec_version == spec_version : desired_extension.spec_version <= spec_version;
        if (found && valid_version) {
            filtered_extensions.push_back(desired_extension);
        }
        else {
            std::string version_info;
            if (desired_extension.spec_version != 0 || desired_extension.exact_spec_version) {
                version_info = " (v." + std::to_string(spec_version) + " " + ((spec_version != 0u) ? "== " : ">= ") + std::to_string(desired_extension.spec_version) + ")";
            }
            if (desired_extension.required) {
                all_found = false;
            }
            VK_TEST_SAY((desired_extension.required ? "ERROR" : "WARNING") << " : " << "\nExtension not available : " << desired_extension.extension_name << "\nVersion info : " << version_info.c_str());
        }
    }

    return all_found;
}

VkResult vk_test::Context::getDeviceExtensions(VkPhysicalDevice physical_device, std::vector<VkExtensionProperties>& extension_properties) {
    uint32_t count{};
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, nullptr);
    extension_properties.resize(count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &count, extension_properties.data());
    extension_properties.resize(std::min(extension_properties.size(), size_t(count)));
    return VK_SUCCESS;
}
