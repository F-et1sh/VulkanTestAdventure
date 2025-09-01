#include "pch.h"
#include "Application.h"

#undef max

void Application::InitializeWindow() {
    if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Window = glfwCreateWindow(800, 600, "VulkanTestAdventure", nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
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

    auto present_index = m_PhysicalDevice.getSurfaceSupportKHR(graphics_index, *m_Surface) ? graphics_index : static_cast<uint32_t>(queue_family_properties.size());

    if (present_index == queue_family_properties.size()) {
        for (size_t i = 0; i < queue_family_properties.size(); i++) {

            if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) && m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_Surface)) {
                graphics_index = static_cast<uint32_t>(i);
                present_index = graphics_index;
                break;
            }
        }
        if (present_index == queue_family_properties.size()) {
            for (size_t i = 0; i < queue_family_properties.size(); i++) {
                if (m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_Surface)) {
                    present_index = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }

    if ((graphics_index == queue_family_properties.size()) || (present_index == queue_family_properties.size()))
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
    m_PresentQueue = vk::raii::Queue(m_Device, present_index, 0);

    m_GraphicsQueueFamilyIndex = graphics_index;
    m_PresentQueueFamilyIndex = present_index;
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
        vk::True,
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

void Application::CreateCommandBuffers() {
    m_CommandBuffers.clear();
    vk::CommandBufferAllocateInfo alloc_info{ m_CommandPool, vk::CommandBufferLevel::ePrimary, MAX_FRAMES_IN_FLIGHT };
    m_CommandBuffers = vk::raii::CommandBuffers(m_Device, alloc_info);
}

void Application::CreateSyncObjects() {
    m_PresentCompleteSemaphores.clear();
    m_RenderFinishedSemaphores.clear();
    m_InFlightFences.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_PresentCompleteSemaphores.emplace_back(m_Device, vk::SemaphoreCreateInfo{});
        m_RenderFinishedSemaphores.emplace_back(m_Device, vk::SemaphoreCreateInfo{});
        m_InFlightFences.emplace_back(m_Device, vk::FenceCreateInfo{ vk::FenceCreateFlagBits::eSignaled });
    }
}

void Application::DrawFrame() {
    vk::Result wait_result = m_Device.waitForFences(*m_InFlightFences[m_CurrentFrame], vk::True, UINT64_MAX);
    if (wait_result != vk::Result::eSuccess) {
        throw std::runtime_error("ERROR : waitForFences failed");
    }

    //auto [result, image_index] = m_Swapchain.acquireNextImage(UINT64_MAX, m_PresentCompleteSemaphores[m_CurrentFrame], nullptr);

    uint32_t image_index = 0;
    VkResult result = vkAcquireNextImageKHR(*m_Device, *m_Swapchain, UINT64_MAX, *m_PresentCompleteSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        this->RecreateSwapchain();
        return;
    }
    else if (result != 0 && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("ERROR : failed to acquire swap chain image");
    }

    m_Device.resetFences(*m_InFlightFences[m_CurrentFrame]);

    m_CommandBuffers[m_CurrentFrame].reset();
    this->RecordCommandBuffer(m_CommandBuffers[m_CurrentFrame], image_index);

    vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo{
        1,
        &* m_PresentCompleteSemaphores[m_CurrentFrame],
        & wait_stages,
        1,
        &*m_CommandBuffers[m_CurrentFrame],
        1,
        &*m_RenderFinishedSemaphores[m_CurrentFrame]
    };

    m_GraphicsQueue.submit(submitInfo, m_InFlightFences[m_CurrentFrame]);

    vk::PresentInfoKHR present_info{
        1,
        &*m_RenderFinishedSemaphores[m_CurrentFrame],
        1,
        &*m_Swapchain,
        & image_index
    };

    //result = m_GraphicsQueue.presentKHR(present_info);
    result = vkQueuePresentKHR(*m_GraphicsQueue, present_info);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        RecreateSwapchain();
    }
    else if (result != 0)
        throw std::runtime_error("ERROR : Failed to present swap chain image");

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::ReleaseSwapchain() {
    m_GraphicsPipeline = nullptr;
    m_PipelineLayout = nullptr;
    m_SwapchainImageViews.clear();
    m_Swapchain = nullptr;
}

void Application::RecreateSwapchain() {
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window, &width, &height);
        glfwWaitEvents();
    }

    m_Device.waitIdle();

    this->ReleaseSwapchain();

    this->CreateSwapchain();
    this->CreateImageViews();
    this->CreateGraphicsPipeline();

    this->CreateCommandBuffers();
    
    m_CurrentFrame = 0;
    m_FramebufferResized = false;
}

void Application::FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->m_FramebufferResized = true;
}

void Application::MainLoop() {
    using clock = std::chrono::high_resolution_clock;
    auto last_time = clock::now();
    int frames = 0;

    while (!glfwWindowShouldClose(m_Window)) {
        glfwPollEvents();
        this->DrawFrame();

        frames++;
        auto now = clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_time);

        if (duration.count() >= 1) {
            frames = 0;
            last_time = now;
        }
    }

    m_Device.waitIdle();
}

void Application::Release() {
    this->ReleaseSwapchain();

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Application::CreateSurface() {
    VkSurfaceKHR surface{};
    if (glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &surface) != 0)
        throw std::runtime_error("ERROR : Failed to create window surface");
    m_Surface = vk::raii::SurfaceKHR(m_Instance, surface);
}

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

void Application::RecordCommandBuffer(vk::raii::CommandBuffer& command_buffer, uint32_t image_index) {
    command_buffer.begin(vk::CommandBufferBeginInfo{});

    TransitionImageLayout(
        command_buffer,
        image_index,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::AccessFlagBits2KHR::eNone,
        vk::AccessFlagBits2KHR::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2KHR::eTopOfPipe,
        vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput
    );

    vk::ClearValue clear_color = vk::ClearColorValue(0.05f, 0.05f, 0.05f, 1.0f);
    vk::RenderingAttachmentInfo attachment_info{
        m_SwapchainImageViews[image_index],
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        {},
        {},
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        clear_color
    };

    vk::RenderingInfo rendering_info{
        vk::RenderingFlags{},
        {{ 0, 0 }, m_SwapchainExtent },
        1,
        {},
        1,
        &attachment_info
    };

    command_buffer.beginRendering(rendering_info);

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_GraphicsPipeline);
    command_buffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_SwapchainExtent.width), static_cast<float>(m_SwapchainExtent.height), 0.0f, 1.0f));
    command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_SwapchainExtent));

    command_buffer.draw(3, 1, 0, 0);

    command_buffer.endRendering();

    TransitionImageLayout(
        command_buffer,
        image_index,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eNone
    );

    command_buffer.end();
}

void Application::TransitionImageLayout(
    vk::raii::CommandBuffer& command_buffer,
    uint32_t image_index,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask)
    const {

    vk::ImageMemoryBarrier2 barrier = {
            src_stage_mask,
            src_access_mask,
            dst_stage_mask,
            dst_access_mask,
            old_layout,
            new_layout,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            m_SwapchainImages[image_index],
            {
                vk::ImageAspectFlagBits::eColor,
                0,
                1,
                0,
                1
            }
    };

    vk::DependencyInfo dependency_info{ {}, {}, {}, {}, {}, 1, &barrier };
    command_buffer.pipelineBarrier2(dependency_info);
}
