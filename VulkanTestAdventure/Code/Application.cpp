#include "pch.h"
#include "Application.h"

#undef max

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

constexpr static std::array<const char*, 4> DEVICE_EXTENSIONS = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName
};

void Application::PickPhysicalDevice() {
    std::vector<vk::raii::PhysicalDevice> devices = m_Instance.enumeratePhysicalDevices();
    const auto device_iterator = std::ranges::find_if(devices,
        [&](auto const& device) {
            auto queue_families = device.getQueueFamilyProperties();
            bool is_suitable = device.getProperties().apiVersion >= VK_API_VERSION_1_3;
            
            const auto qfp_iterator = std::ranges::find_if(queue_families, [](vk::QueueFamilyProperties const& qfp) {
                return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
                });

            is_suitable = is_suitable && (qfp_iterator != queue_families.end());
            auto extensions = device.enumerateDeviceExtensionProperties();
            bool found = true;

            for (auto const& extension : DEVICE_EXTENSIONS) {
                auto extension_iterator = std::ranges::find_if(extensions, [extension](auto const& ext) {return strcmp(ext.extensionName, extension) == 0; });
                found = found && extension_iterator != extensions.end();
            }
            
            is_suitable = is_suitable && found;
            if (is_suitable) m_PhysicalDevice = device;

            return is_suitable;
        });

    if (device_iterator == devices.end()) throw std::runtime_error("ERROR : Failed to find a suitable GPU");
}

void Application::CreateLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queue_family_properties = m_PhysicalDevice.getQueueFamilyProperties();

    auto graphics_queue_family_property = std::ranges::find_if(
        queue_family_properties,
        [](auto const& qfp) { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); }
    );

    uint32_t graphics_index = static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_family_property));

    auto presentIndex = m_PhysicalDevice.getSurfaceSupportKHR(graphics_index, *m_Surface) ? graphics_index : static_cast<uint32_t>(queue_family_properties.size());

    if (presentIndex == queue_family_properties.size()) {
        for (size_t i = 0; i < queue_family_properties.size(); i++) {

            if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) && m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_Surface)) {
                graphics_index = static_cast<uint32_t>(i);
                presentIndex = graphics_index;
                break;
            }
        }
        if (presentIndex == queue_family_properties.size()) {
            for (size_t i = 0; i < queue_family_properties.size(); i++) {
                if (m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_Surface)) {
                    presentIndex = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }

    if ((graphics_index == queue_family_properties.size()) || (presentIndex == queue_family_properties.size()))
        throw std::runtime_error("ERROR : Could not find a queue for graphics or present");

    auto features = m_PhysicalDevice.getFeatures2();
    vk::PhysicalDeviceVulkan13Features vulkan13_features;
    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features;
    vulkan13_features.dynamicRendering = vk::True;
    extended_dynamic_state_features.extendedDynamicState = vk::True;
    vulkan13_features.pNext = &extended_dynamic_state_features;
    features.pNext = &vulkan13_features;

    float queue_priority = 0.0f;
    vk::DeviceQueueCreateInfo device_queue_create_info{};
    device_queue_create_info.setQueueFamilyIndex(graphics_index);
    device_queue_create_info.setQueueCount(1);
    device_queue_create_info.setPQueuePriorities(&queue_priority);

    vk::DeviceCreateInfo device_create_info{};
    device_create_info.setPNext(&features);
    device_create_info.setQueueCreateInfoCount(1);
    device_create_info.setPQueueCreateInfos(&device_queue_create_info);
    device_create_info.setEnabledExtensionCount(DEVICE_EXTENSIONS.size());
    device_create_info.setPpEnabledExtensionNames(DEVICE_EXTENSIONS.data());

    device_create_info.setEnabledExtensionCount(DEVICE_EXTENSIONS.size());
    device_create_info.setPpEnabledExtensionNames(DEVICE_EXTENSIONS.data());

    m_Device = vk::raii::Device(m_PhysicalDevice, device_create_info);
    m_GraphicsQueue = vk::raii::Queue(m_Device, graphics_index, 0);
    m_PresentQueue = vk::raii::Queue(m_Device, presentIndex, 0);

    m_GraphicsQueueFamilyIndex = graphics_index;
    m_PresentQueueFamilyIndex = presentIndex;
}

void Application::CreateSwapchain() {
    auto surface_capabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface);
    auto swapchain_surface_format = ChooseSwapSurfaceFormat(m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface));
    auto swapchain_extent = ChooseSwapExtent(surface_capabilities);

    uint32_t image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount)
        image_count = surface_capabilities.maxImageCount;
    
    uint32_t queue_family_indices[] = { m_GraphicsQueueFamilyIndex, m_PresentQueueFamilyIndex };
    bool same_queue = (m_GraphicsQueueFamilyIndex == m_PresentQueueFamilyIndex);

    vk::SharingMode sharing_mode = same_queue ?
        vk::SharingMode::eExclusive :
        vk::SharingMode::eConcurrent;

    constexpr uint32_t image_array_layers = 1;

    vk::SwapchainCreateInfoKHR swapchain_create_info{
        vk::SwapchainCreateFlagsKHR{},
        m_Surface,
        image_count,
        swapchain_surface_format.format,
        swapchain_surface_format.colorSpace,
        swapchain_extent,
        image_array_layers,
        vk::ImageUsageFlagBits::eColorAttachment,
        sharing_mode,
        same_queue ? 0u : 2u,
        same_queue ? nullptr : queue_family_indices,
        surface_capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        ChooseSwapPresentMode(m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface)),
        {},
        nullptr
    };

    m_Swapchain = vk::raii::SwapchainKHR(m_Device, swapchain_create_info);
    m_SwapchainImages = m_Swapchain.getImages();

    m_SwapchainImageFormat = swapchain_surface_format.format;
    m_SwapchainExtent = swapchain_extent;
}

void Application::CreateImageViews() {
    m_SwapchainImageViews.clear();

    vk::ImageViewCreateInfo image_view_create_info{
        vk::ImageViewCreateFlags{},
        vk::Image{},
        vk::ImageViewType::e2D,
        m_SwapchainImageFormat,
        vk::ComponentMapping{},
        { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
        nullptr
    };

    image_view_create_info.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    image_view_create_info.subresourceRange.setBaseMipLevel(0);
    image_view_create_info.subresourceRange.setLevelCount(1);
    image_view_create_info.subresourceRange.setBaseArrayLayer(0);
    image_view_create_info.subresourceRange.setLayerCount(1);

    for (auto image : m_SwapchainImages) {
        image_view_create_info.image = image;
        m_SwapchainImageViews.emplace_back(m_Device, image_view_create_info);
    }
}

void Application::CreateGraphicsPipeline() {
    auto code_vert = LoadShader(L"C:/Users/Пользователь/Desktop/VulkanTestAdventure/Files/Shaders/Test1/shader.vert.spv");
    auto code_frag = LoadShader(L"C:/Users/Пользователь/Desktop/VulkanTestAdventure/Files/Shaders/Test1/shader.frag.spv");

    vk::raii::ShaderModule shader_module_vert = this->CreateShaderModule(code_vert);
    vk::raii::ShaderModule shader_module_frag = this->CreateShaderModule(code_frag);

    vk::PipelineShaderStageCreateInfo vert_shader_stage_info{ vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eVertex, shader_module_vert, "main" };
    vk::PipelineShaderStageCreateInfo frag_shader_stage_info{ vk::PipelineShaderStageCreateFlags{}, vk::ShaderStageFlagBits::eFragment, shader_module_frag, "main" };

    vk::PipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

    std::vector dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{ vk::PipelineDynamicStateCreateFlags{}, static_cast<uint32_t>(dynamic_states.size()), dynamic_states.data() };

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vk::PipelineInputAssemblyStateCreateInfo input_assembly{ vk::PipelineInputAssemblyStateCreateFlags{}, vk::PrimitiveTopology::eTriangleList };

    vk::Viewport{ 0.0f, 0.0f, static_cast<float>(m_SwapchainExtent.width), static_cast<float>(m_SwapchainExtent.height), 0.0f, 1.0f };
    vk::Rect2D{ vk::Offset2D{ 0, 0 }, m_SwapchainExtent };

    vk::PipelineViewportStateCreateInfo viewport_state{ vk::PipelineViewportStateCreateFlags{}, 1, nullptr, 1, nullptr };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        vk::PipelineRasterizationStateCreateFlags{},
        vk::False,
        vk::False,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        vk::False,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{ vk::PipelineMultisampleStateCreateFlags{}, vk::SampleCountFlagBits::e1, vk::False };

    vk::PipelineColorBlendAttachmentState color_blend_attachment{
        vk::True,
        vk::BlendFactor::eSrcAlpha,
        vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo color_blending{ vk::PipelineColorBlendStateCreateFlags{}, vk::False, vk::LogicOp::eCopy, 1, &color_blend_attachment };

    vk::PipelineLayoutCreateInfo pipeline_layout_info{};

    m_PipelineLayout = vk::raii::PipelineLayout(m_Device, pipeline_layout_info);

    vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{ {}, 1, &m_SwapchainImageFormat };

    vk::GraphicsPipelineCreateInfo pipeline_info{
        vk::PipelineCreateFlags{},
        static_cast<uint32_t>(std::size(shader_stages)),
        shader_stages,
        &vertex_input_info,
        &input_assembly,
        nullptr,
        &viewport_state,
        &rasterizer,
        &multisampling,
        nullptr,
        &color_blending,
        &dynamic_state,
        *m_PipelineLayout,
        nullptr,
        0,
        nullptr,
        -1,
        &pipeline_rendering_create_info
    };

    m_GraphicsPipeline = vk::raii::Pipeline(m_Device, nullptr, pipeline_info);
}

void Application::CreateCommandPool() {
    vk::CommandPoolCreateInfo pool_info{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_GraphicsQueueFamilyIndex };
    m_CommandPool = vk::raii::CommandPool(m_Device, pool_info);
}

void Application::CreateSurface() {
    VkSurfaceKHR surface{};
    if (glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &surface) != 0)
        throw std::runtime_error("ERROR : Failed to create window surface");
    m_Surface = vk::raii::SurfaceKHR(m_Instance, surface);
}

constexpr static std::array<const char*, 1> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

void Application::EnableValidationLayers(vk::InstanceCreateInfo& create_info) const {
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

uint32_t Application::FindQueueFamilies() const {
    std::vector<vk::QueueFamilyProperties> queue_family_properties = m_PhysicalDevice.getQueueFamilyProperties();

    auto graphics_queue_family_property = std::find_if(queue_family_properties.begin(), queue_family_properties.end(),
        [](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & vk::QueueFlagBits::eGraphics; });

    return static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_family_property));
}

vk::SurfaceFormatKHR Application::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats)const {
    for (const auto& availableFormat : available_formats)
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            return availableFormat;
    return available_formats[0];
}

vk::PresentModeKHR Application::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes)const {
    for (const auto& available_present_mode : available_present_modes)
        if (available_present_mode == vk::PresentModeKHR::eMailbox)
            return available_present_mode;
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Application::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);

    return {
        std::clamp<uint32_t>(width , capabilities.minImageExtent.width , capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

std::vector<char> Application::LoadShader(const std::filesystem::path& path)const {
    std::ifstream file(path, std::ios::binary);
    if (!file.good()) {
        std::cerr << "ERROR : Failed to load shader file\nPath : " << path << std::endl;
        return {};
    }
    return { (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>() };
}

vk::raii::ShaderModule Application::CreateShaderModule(const std::vector<char>& code)const {
    vk::ShaderModuleCreateInfo create_info{ vk::ShaderModuleCreateFlags{}, code.size() * sizeof(char), reinterpret_cast<const uint32_t*>(code.data())};
    vk::raii::ShaderModule shader_module{ m_Device, create_info };
    return shader_module;
}
