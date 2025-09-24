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

}

void VKTest::DeviceManager::CreateLogicalDevice() {

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