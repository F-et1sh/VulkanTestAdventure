#include "Application.h"

void Application::InitializeWindow() {
    if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Window = glfwCreateWindow(800, 600, "VulkanTestAdventure", nullptr, nullptr);
}

void Application::CreateInstance() {
    constexpr vk::ApplicationInfo app_info{ "VulkanTestAdventure", VK_MAKE_VERSION(1, 0, 0), "No Engine", VK_MAKE_VERSION(1, 0, 0), vk::ApiVersion14 };

    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    auto extension_properties = m_Context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfw_extension_count; i++) {
        
        auto lambda = [glfw_extension = glfw_extensions[i]](auto const& extension_property) {
            return strcmp(extension_property.extensionName, glfw_extension) == 0;
            };

        if (std::ranges::none_of(extension_properties, lambda))
            throw std::runtime_error("ERROR : Required GLFW extension not supported : " + std::string(glfw_extensions[i]));
    }

    vk::InstanceCreateInfo create_info{ vk::InstanceCreateFlags{}, &app_info, 0, nullptr, glfw_extension_count, glfw_extensions };

#ifdef NDEBUG
    // nothing here..
#else
    this->EnableValidationLayers(create_info);
#endif

    m_Instance = vk::raii::Instance(m_Context, create_info);
}

void Application::EnableValidationLayers(vk::InstanceCreateInfo& create_info) const {
    const std::vector<const char*> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

    auto available_layers = m_Context.enumerateInstanceLayerProperties();

    bool layers_supported = true;
    for (const auto& layer_name : VALIDATION_LAYERS) {
        bool layer_found = false;
        for (const auto& layer : available_layers) {
            if (!strcmp(layer.layerName, layer_name)) {
                layer_found = true;
                break;
            }
        }
        if (!layer_found) {
            layers_supported = false;
            std::cerr << "ERROR : Validation Layer not found : " << layer_name << std::endl;
            break;
        }
    }

    if (!layers_supported) throw std::runtime_error("ERROR : Validation layers requested, but not available!");
    if (!VALIDATION_LAYERS.empty()) create_info.setPpEnabledLayerNames(VALIDATION_LAYERS.data());
}
