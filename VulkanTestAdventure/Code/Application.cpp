#include "pch.h"
#include "Application.h"

// STB Image
#define STB_IMAGE_IMPLEMENTATION
#include "STB/stb_image.h"

// Tiny Obj Loader
#define TINYOBJLOADER_IMPLEMENTATION
#include "TinyObjLoader/tiny_obj_loader.h"

#undef max

void Application::InitializeWindow() {
    if (!glfwInit()) throw std::runtime_error("Failed to init GLFW");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_Window = glfwCreateWindow(1920, 1080, "VulkanTestAdventure", nullptr, nullptr);
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
}

void Application::CreateInstance() {
    constexpr vk::ApplicationInfo app_info{ "VulkanTestAdventure", VK_MAKE_VERSION(1, 0, 0), "No Engine", VK_MAKE_VERSION(1, 0, 0), vk::ApiVersion14 };

    std::vector<const char*> extensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
    
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    auto extension_properties = m_Context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < glfw_extension_count; i++) {
        
        auto lambda = [glfw_extension = glfw_extensions[i]](auto const& extension_property) {
            return strcmp(extension_property.extensionName, glfw_extension) == 0;
            };

        if (std::ranges::none_of(extension_properties, lambda))
            throw std::runtime_error("Required GLFW extension not supported : " + std::string(glfw_extensions[i]));

        extensions.emplace_back(glfw_extensions[i]);
    }

    vk::InstanceCreateInfo create_info{ vk::InstanceCreateFlags{}, &app_info, 0, nullptr, static_cast<uint32_t>(extensions.size()), extensions.data() };

#ifdef NDEBUG
    // nothing here..
#else
    this->EnableValidationLayers(create_info);
#endif

    m_Instance = vk::raii::Instance(m_Context, create_info);

#ifdef NDEBUG
    // nothing here..
#else
    vk::DebugUtilsMessengerCreateInfoEXT messenger_create_info{
        vk::DebugUtilsMessengerCreateFlagsEXT{},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        DebugCallback
    };

    m_DebugMessenger = m_Instance.createDebugUtilsMessengerEXT(messenger_create_info);
#endif
}

void Application::PickPhysicalDevice() {
    std::vector<vk::raii::PhysicalDevice> devices = m_Instance.enumeratePhysicalDevices();
    const auto device_iterator = std::ranges::find_if(devices,
        [&](auto const& device) {
            auto queue_families = device.getQueueFamilyProperties();
            bool is_suitable = device.getProperties().apiVersion >= VK_API_VERSION_1_3;
            
            const auto qfp_iterator = std::ranges::find_if(queue_families, [](vk::QueueFamilyProperties const& qfp) {
                return (qfp.queueFlags & REQUIRED_QUEUE_FAMILY_FLAGS) != static_cast<vk::QueueFlags>(0);
                });

            is_suitable = is_suitable && (qfp_iterator != queue_families.end());
            auto extensions = device.enumerateDeviceExtensionProperties();
            bool found = true;

            for (auto const& extension : DEVICE_EXTENSIONS) {
                auto extension_iterator = std::ranges::find_if(extensions, [extension](auto const& ext) {return strcmp(ext.extensionName, extension) == 0; });
                found = found && extension_iterator != extensions.end();
            }
            
            is_suitable = is_suitable && found;
            
            const auto& features = device.getFeatures();
            is_suitable = features.samplerAnisotropy;

            if (is_suitable) m_PhysicalDevice = device;

            return is_suitable;
        });

    if (device_iterator == devices.end()) throw std::runtime_error("Failed to find a suitable GPU");
}

void Application::CreateLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queue_family_properties = m_PhysicalDevice.getQueueFamilyProperties();

    auto graphics_queue_family_property = std::ranges::find_if(
        queue_family_properties,
        [](auto const& qfp) { return (qfp.queueFlags & REQUIRED_QUEUE_FAMILY_FLAGS) != static_cast<vk::QueueFlags>(0); }
    );

    uint32_t graphics_index = static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_family_property));

    auto present_index = m_PhysicalDevice.getSurfaceSupportKHR(graphics_index, *m_Surface) ? graphics_index : static_cast<uint32_t>(queue_family_properties.size());

    if (present_index == queue_family_properties.size()) {
        for (size_t i = 0; i < queue_family_properties.size(); i++) {

            if ((queue_family_properties[i].queueFlags & REQUIRED_QUEUE_FAMILY_FLAGS) && m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_Surface)) {
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
        throw std::runtime_error("Could not find a queue for graphics or present");

    vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state_features;
    extended_dynamic_state_features.extendedDynamicState = vk::True;
    
    vk::PhysicalDeviceVulkan13Features vulkan13_features;
    vulkan13_features.dynamicRendering = vk::True;
    vulkan13_features.synchronization2 = vk::True;
    vulkan13_features.pNext = &extended_dynamic_state_features;
    
    auto features = m_PhysicalDevice.getFeatures2();
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

    constexpr vk::SharingMode sharing_mode = vk::SharingMode::eExclusive;
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

    for (auto image : m_SwapchainImages) {
        image_view_create_info.image = image;
        m_SwapchainImageViews.emplace_back(m_Device, image_view_create_info);
    }
}

void Application::CreateDescriptorSetLayout() {
    constexpr static std::array bindings = {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment, nullptr)
    };

    vk::DescriptorSetLayoutCreateInfo layout_info{ vk::DescriptorSetLayoutCreateFlags{}, bindings.size(), bindings.data() };
    m_DescriptorSetLayout = vk::raii::DescriptorSetLayout(m_Device, layout_info);
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

    auto binding_description = Vertex::getBindingDescription();
    auto attribute_descriptions = Vertex::getAttributeDescriptions();

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{ vk::PipelineVertexInputStateCreateFlags{}, 1, &binding_description, attribute_descriptions.size(), attribute_descriptions.data() };
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
        vk::FrontFace::eCounterClockwise,
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

    vk::PipelineLayoutCreateInfo pipeline_layout_info{ vk::PipelineLayoutCreateFlags{}, 1, &*m_DescriptorSetLayout, 0 };
    m_PipelineLayout = vk::raii::PipelineLayout(m_Device, pipeline_layout_info);

    vk::Format depth_format = this->FindDepthFormat();
    vk::PipelineRenderingCreateInfo pipeline_rendering_create_info{
        0,
        1,
        &m_SwapchainImageFormat,
        depth_format,
        vk::Format::eUndefined
    };

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state{
        vk::PipelineDepthStencilStateCreateFlags{},
        vk::True,
        vk::True,
        vk::CompareOp::eLess,
        vk::False,
        vk::False,
        {},
        {},
        0.0f,
        1.0f
    };

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
        &depth_stencil_state,
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

void Application::CreateDepthResources() {
    vk::Format depth_format = this->FindDepthFormat();
    this->CreateImage(m_SwapchainExtent.width, m_SwapchainExtent.height, 1, depth_format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, m_DepthImage, m_DepthImageMemory);
    m_DepthImageView = this->CreateImageView(m_DepthImage, depth_format, vk::ImageAspectFlagBits::eDepth, 1);
}

void Application::CreateTextureImage() {
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    vk::DeviceSize image_size = width * height * 4;

    if (!pixels)
        throw std::runtime_error(std::string("Failed to load texture image\nPath : ") + TEXTURE_PATH);

    m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    vk::raii::Buffer staging_buffer{ {} };
    vk::raii::DeviceMemory staging_buffer_memory{ {} };

    this->CreateBuffer(image_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, staging_buffer, staging_buffer_memory);

    void* data = staging_buffer_memory.mapMemory(0, image_size);
    memcpy(data, pixels, image_size);
    staging_buffer_memory.unmapMemory();

    stbi_image_free(pixels);

    this->CreateImage(width, height, m_MipLevels, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, m_TextureImage, m_TextureImageMemory);

    TransitionTextureImageLayout(m_TextureImage, {}, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, m_MipLevels);

    CopyBufferToImage(staging_buffer, m_TextureImage, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    TransitionTextureImageLayout(m_TextureImage, {}, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, m_MipLevels);

    this->GenerateMipmaps(m_TextureImage, vk::Format::eR8G8B8A8Srgb, width, height, m_MipLevels);
}

void Application::CreateTextureImageView() {
    m_TextureImageView = this->CreateImageView(m_TextureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, m_MipLevels);
}

void Application::CreateTextureSampler() {
    vk::PhysicalDeviceProperties properties = m_PhysicalDevice.getProperties();
    vk::SamplerCreateInfo sampler_info{
        vk::SamplerCreateFlags{},
        vk::Filter::eLinear,
        vk::Filter::eLinear,
        vk::SamplerMipmapMode::eLinear,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        0.0f,
        vk::True,
        properties.limits.maxSamplerAnisotropy,
        vk::False,
        vk::CompareOp::eAlways
    };

    m_TextureSampler = vk::raii::Sampler(m_Device, sampler_info);
}

void Application::LoadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str()))
        throw std::runtime_error(err);

    m_Vertices.reserve(shapes.size());
    m_Indices.reserve(shapes.size());

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texture_coord = {
                 attrib.texcoords[2 * index.texcoord_index + 0],
                 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };
            
            m_Vertices.emplace_back(vertex);
            m_Indices.emplace_back(m_Indices.size());
        }
    }
}

void Application::CreateVertexBuffer() {
    vk::DeviceSize buffer_size = sizeof(m_Vertices[0]) * m_Vertices.size();

    vk::raii::Buffer staging_buffer{ {} };
    vk::raii::DeviceMemory staging_buffer_memory{ {} };
    this->CreateBuffer(buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, staging_buffer, staging_buffer_memory);

    void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
    memcpy(data_staging, m_Vertices.data(), buffer_size);
    staging_buffer_memory.unmapMemory();

    this->CreateBuffer(buffer_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_VertexBuffer, m_VertexBufferMemory);
    this->CopyBuffer(staging_buffer, m_VertexBuffer, buffer_size);
}

void Application::CreateIndexBuffer() {
    vk::DeviceSize buffer_size = sizeof(m_Indices[0]) * m_Indices.size();

    vk::raii::Buffer staging_buffer{ {} };
    vk::raii::DeviceMemory staging_buffer_memory{ {} };
    CreateBuffer(buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, staging_buffer, staging_buffer_memory);

    void* data_staging = staging_buffer_memory.mapMemory(0, buffer_size);
    memcpy(data_staging, m_Indices.data(), buffer_size);
    staging_buffer_memory.unmapMemory();

    this->CreateBuffer(buffer_size, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_IndexBuffer, m_IndexBufferMemory);
    this->CopyBuffer(staging_buffer, m_IndexBuffer, buffer_size);
}

void Application::CreateUniformBuffers() {
    m_UniformBuffers.clear();
    m_UniformBuffersMemory.clear();
    m_UniformBuffersMapped.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize buffer_size = sizeof(UniformBufferObject);
        vk::raii::Buffer buffer{ {} };
        vk::raii::DeviceMemory buffer_memory{ {} };
        this->CreateBuffer(buffer_size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, buffer_memory);
        m_UniformBuffers.emplace_back(std::move(buffer));
        m_UniformBuffersMemory.emplace_back(std::move(buffer_memory));
        m_UniformBuffersMapped.emplace_back(m_UniformBuffersMemory[i].mapMemory(0, buffer_size));
    }
}

void Application::CreateDescriptorPool() {
    constexpr static std::array pool_size{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT)
    };
    vk::DescriptorPoolCreateInfo pool_info(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, MAX_FRAMES_IN_FLIGHT, pool_size);

    m_DescriptorPool = vk::raii::DescriptorPool(m_Device, pool_info);
}

void Application::CreateDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *m_DescriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocate_info{ m_DescriptorPool, static_cast<uint32_t>(layouts.size()), layouts.data() };
    
    m_DescriptorSets.clear();
    m_DescriptorSets = m_Device.allocateDescriptorSets(allocate_info);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo buffer_info{ m_UniformBuffers[i], 0, sizeof(UniformBufferObject) };
        vk::DescriptorImageInfo image_info(m_TextureSampler, m_TextureImageView, vk::ImageLayout::eShaderReadOnlyOptimal);

        std::array descriptor_writes{
            vk::WriteDescriptorSet(m_DescriptorSets[i], 0, 0, 1, vk::DescriptorType::eUniformBuffer, nullptr, &buffer_info),
            vk::WriteDescriptorSet(m_DescriptorSets[i], 1, 0, 1, vk::DescriptorType::eCombinedImageSampler, &image_info)
        };
        m_Device.updateDescriptorSets(descriptor_writes, {});
    }
}

void Application::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& buffer_memory)const {
    vk::BufferCreateInfo buffer_info{ vk::BufferCreateFlags{}, size, usage, vk::SharingMode::eExclusive };
    buffer = vk::raii::Buffer(m_Device, buffer_info);
    vk::MemoryRequirements memory_requirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocate_info{ memory_requirements.size, FindMemoryType(memory_requirements.memoryTypeBits, properties) };
    buffer_memory = vk::raii::DeviceMemory(m_Device, allocate_info);
    buffer.bindMemory(*buffer_memory, 0);
}

void Application::CopyBuffer(vk::raii::Buffer& src_buffer, vk::raii::Buffer& dst_buffer, vk::DeviceSize size) {
    vk::raii::CommandBuffer commandCopyBuffer = BeginSingleTimeCommands();
    commandCopyBuffer.copyBuffer(src_buffer, dst_buffer, vk::BufferCopy(0, 0, size));
    EndSingleTimeCommands(commandCopyBuffer);
}

void Application::UpdateUniformBuffer(uint32_t current_image) {
    UniformBufferObject ubo{};

    double mouse_x = 0.0;
    double mouse_y = 0.0;
    glfwGetCursorPos(m_Window, &mouse_x, &mouse_y);

    float normalized_mouse_x = static_cast<float>(mouse_x - m_SwapchainExtent.width / 2.0) / (m_SwapchainExtent.width / 2.0);
    float normalized_mouse_y = static_cast<float>(mouse_y - m_SwapchainExtent.height / 2.0) / (m_SwapchainExtent.height / 2.0);

    float rotation_speed = 1.0f;
    float yaw = normalized_mouse_x * glm::radians(180.0f) * rotation_speed;
    float pitch = normalized_mouse_y * glm::radians(90.0f) * rotation_speed;

    static glm::vec3 position{ 0.0f, 0.0f, 2.0f };

    float camera_speed = 0.001f;
    if (glfwGetKey(m_Window, GLFW_KEY_W))
        position += camera_speed * glm::vec3(0.0f, 0.0f, -1.0f);
    if (glfwGetKey(m_Window, GLFW_KEY_S))
        position -= camera_speed * glm::vec3(0.0f, 0.0f, -1.0f);
    if (glfwGetKey(m_Window, GLFW_KEY_A))
        position += camera_speed * glm::vec3(-1.0f, 0.0f, 0.0f);
    if (glfwGetKey(m_Window, GLFW_KEY_D))
        position += camera_speed * glm::vec3(1.0f, 0.0f, 0.0f);
    if (glfwGetKey(m_Window, GLFW_KEY_Q))
        position += camera_speed * glm::vec3(0.0f, 1.0f, 0.0f);
    if (glfwGetKey(m_Window, GLFW_KEY_E))
        position += camera_speed * glm::vec3(0.0f, -1.0f, 0.0f);


    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::rotate(modelMatrix, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, pitch, glm::vec3(1.0f, 0.0f, 0.0f));

    ubo.model = modelMatrix;

    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
    ubo.view = glm::lookAt(position, center, glm::vec3(0.0f, 1.0f, 0.0f));

    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(m_SwapchainExtent.width) / static_cast<float>(m_SwapchainExtent.height), 0.1f, 100.0f);
    ubo.proj[1][1] *= -1;

    memcpy(m_UniformBuffersMapped[current_image], &ubo, sizeof(ubo));
}

void Application::CreateImage(uint32_t width, uint32_t height, uint32_t mip_levels, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image, vk::raii::DeviceMemory& image_memory)const {
    vk::ImageCreateInfo image_info{ vk::ImageCreateFlags{}, vk::ImageType::e2D, format, { width, height, 1 }, mip_levels, 1, vk::SampleCountFlagBits::e1, tiling, usage, vk::SharingMode::eExclusive, 0 };

    image = vk::raii::Image(m_Device, image_info);

    vk::MemoryRequirements memory_requirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocate_info(memory_requirements.size, FindMemoryType(memory_requirements.memoryTypeBits, properties));
    image_memory = vk::raii::DeviceMemory(m_Device, allocate_info);
    image.bindMemory(image_memory, 0);
}

vk::raii::CommandBuffer Application::BeginSingleTimeCommands()const {
    vk::CommandBufferAllocateInfo allocate_info(m_CommandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::raii::CommandBuffer command_buffers = std::move(m_Device.allocateCommandBuffers(allocate_info).front());

    vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffers.begin(begin_info);

    return command_buffers;
}

void Application::EndSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer)const {
    commandBuffer.end();
    
    vk::raii::Fence copy_fence{ m_Device, vk::FenceCreateInfo{} };

    vk::SubmitInfo submitInfo({}, {}, { *commandBuffer });
    m_GraphicsQueue.submit(submitInfo, copy_fence);
    vk::Result result = m_Device.waitForFences({ *copy_fence }, VK_TRUE, UINT64_MAX);
    if (result != vk::Result::eSuccess)
        throw std::runtime_error("waitForFence failed. " + std::to_string(static_cast<size_t>(result)));
}

void Application::TransitionTextureImageLayout(const vk::Image& image, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout, uint32_t mip_levels)const {
    auto command_buffer = BeginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier{ {}, {}, old_layout, new_layout, {}, {}, image, vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, 0, mip_levels, 0, 1 } };

    vk::PipelineStageFlags source_stage{};
    vk::PipelineStageFlags destination_stage{};

    if (new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

        if (this->HasStencilComponent(format))
            barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
    else barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

    if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        destination_stage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        source_stage = vk::PipelineStageFlagBits::eTransfer;
        destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

        source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
        destination_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    }
    else throw std::invalid_argument("unsupported layout transition!");

    command_buffer.pipelineBarrier(source_stage, destination_stage, vk::DependencyFlags{}, {}, nullptr, barrier);

    EndSingleTimeCommands(command_buffer);
}

void Application::CopyBufferToImage(const vk::raii::Buffer& buffer, const vk::raii::Image& image, uint32_t width, uint32_t height)const {
    auto cmd = BeginSingleTimeCommands();

    vk::BufferImageCopy region(
        0,
        0, 0,
        vk::ImageSubresourceLayers(
            vk::ImageAspectFlagBits::eColor,
            0,
            0,
            1 
        ),
        vk::Offset3D{ 0, 0, 0 },
        vk::Extent3D{ width, height, 1 }
    );

    cmd.copyBufferToImage(*buffer, *image, vk::ImageLayout::eTransferDstOptimal, region);

    EndSingleTimeCommands(cmd);
}

vk::raii::ImageView Application::CreateImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspect_flags, uint32_t mip_levels)const {
    vk::ImageViewCreateInfo view_info{ vk::ImageViewCreateFlags{}, image, vk::ImageViewType::e2D, format, {}, vk::ImageSubresourceRange{ aspect_flags, 0, mip_levels, 0, 1 }};
    return vk::raii::ImageView(m_Device, view_info);
}

vk::Format Application::FindSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)const {
    for (auto format : candidates) {
        vk::FormatProperties properties = m_PhysicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear  && (properties.linearTilingFeatures  & features) == features) return format;
        if (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features) return format;
    }

    throw std::runtime_error("Failed to find supported format");
}

vk::Format Application::FindDepthFormat()const {
    return this->FindSupportedFormat(
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

VKAPI_ATTR vk::Bool32 VKAPI_CALL Application::DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void*) {
    std::cerr << "Validation layer : type " << to_string(type) << " Message : " << callback_data->pMessage << std::endl;
    return vk::False;
}

void Application::DrawFrame() {
    vk::Result wait_result = m_Device.waitForFences(*m_InFlightFences[m_CurrentFrame], vk::True, UINT64_MAX);
    m_Device.resetFences(*m_InFlightFences[m_CurrentFrame]);

    // here I used C-style code because vk::Result, unlike legacy vkResult, works incorrectly and always returns vk::Result::eSuccess

    uint32_t image_index = 0;
    VkResult result = vkAcquireNextImageKHR(*m_Device, *m_Swapchain, UINT64_MAX, *m_PresentCompleteSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        this->RecreateSwapchain();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }

    UpdateUniformBuffer(m_CurrentFrame);
    
    m_CommandBuffers[m_CurrentFrame].reset();
    this->RecordCommandBuffer(m_CommandBuffers[m_CurrentFrame], image_index);

    vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit_info{
        1,
        &*m_PresentCompleteSemaphores[m_CurrentFrame],
        &wait_stages,
        1,
        &*m_CommandBuffers[m_CurrentFrame],
        1,
        &*m_RenderFinishedSemaphores[m_CurrentFrame]
    };

    m_GraphicsQueue.submit(submit_info, m_InFlightFences[m_CurrentFrame]);

    vk::PresentInfoKHR present_info{
        1,
        &*m_RenderFinishedSemaphores[m_CurrentFrame],
        1,
        &*m_Swapchain,
        &image_index
    };

    result = vkQueuePresentKHR(*m_GraphicsQueue, present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        RecreateSwapchain();
    }
    else if (result != VK_SUCCESS) throw std::runtime_error("Failed to present swap chain image");

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::ReleaseSwapchain() {
    m_GraphicsPipeline = nullptr;
    m_PipelineLayout = nullptr;
    m_SwapchainImageViews.clear();
    m_Swapchain = nullptr;
}

void Application::RecreateSwapchain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window, &width, &height);
        glfwWaitEvents();
    }

    m_Device.waitIdle();

    this->ReleaseSwapchain();

    this->CreateSwapchain();
    this->CreateImageViews();

    this->CreateDepthResources();

    this->CreateGraphicsPipeline();

    this->CreateCommandBuffers();

    m_CurrentFrame = 0;
    m_FramebufferResized = false;
}

uint32_t Application::FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties) const {
    vk::PhysicalDeviceMemoryProperties memory_properties = m_PhysicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
        if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    throw std::runtime_error("Failed to find suitable memory type");
}

void Application::GenerateMipmaps(vk::raii::Image& image, vk::Format image_format, int32_t width, int32_t height, uint32_t mip_levels)const {
    vk::FormatProperties format_properties = m_PhysicalDevice.getFormatProperties(image_format);
    if (!(format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
        throw std::runtime_error("Texture image format does not support linear blitting");

    vk::raii::CommandBuffer command_buffer = this->BeginSingleTimeCommands();

    int32_t mip_width = width;
    int32_t mip_height = height;

    for (uint32_t i = 1; i < mip_levels; i++) {
        vk::ImageMemoryBarrier barrier{};
        barrier.image = *image;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            {},
            {},
            {},
            barrier);

        vk::ImageBlit blit{};
        blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        blit.srcOffsets[1] = vk::Offset3D{ mip_width, mip_height, 1 };
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;

        blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        blit.dstOffsets[1] = vk::Offset3D{
            mip_width > 1 ? mip_width / 2 : 1,
            mip_height > 1 ? mip_height / 2 : 1,
            1
        };
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        command_buffer.blitImage(
            *image, vk::ImageLayout::eTransferSrcOptimal,
            *image, vk::ImageLayout::eTransferDstOptimal,
            blit, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
            {}, {}, {}, barrier);

        if (mip_width > 1) mip_width /= 2;
        if (mip_height > 1) mip_height /= 2;
    }

    vk::ImageMemoryBarrier barrier{
        vk::AccessFlagBits::eTransferWrite,
        vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        {},
        {},
        *image,
        vk::ImageSubresourceRange{ vk::ImageAspectFlagBits::eColor, mip_levels - 1, 1, 0, 1 } };

    command_buffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        {},
        {},
        {},
        barrier);

    this->EndSingleTimeCommands(command_buffer);
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
            std::cerr << "FPS : " << frames << std::endl;
            frames = 0;
            last_time = now;
        }
    }
}

void Application::Release() {
    this->m_CommandBuffers.clear();
    this->ReleaseSwapchain();

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Application::CreateSurface() {
    VkSurfaceKHR surface{};
    if (glfwCreateWindowSurface(*m_Instance, m_Window, nullptr, &surface) != 0)
        throw std::runtime_error("Failed to create window surface");
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

    if (!layers_supported) throw std::runtime_error("Validation layers requested, but not available!");
    if (!VALIDATION_LAYERS.empty()) create_info.setPpEnabledLayerNames(VALIDATION_LAYERS.data());
}

uint32_t Application::FindQueueFamilies() const {
    std::vector<vk::QueueFamilyProperties> queue_family_properties = m_PhysicalDevice.getQueueFamilyProperties();

    auto graphics_queue_family_property = std::find_if(queue_family_properties.begin(), queue_family_properties.end(),
        [](vk::QueueFamilyProperties const& qfp) { return qfp.queueFlags & REQUIRED_QUEUE_FAMILY_FLAGS; });

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

    vk::ClearValue clear_color = vk::ClearColorValue(0.01f, 0.01f, 0.01f, 1.0f);
    vk::RenderingAttachmentInfo color_attachment_info{
        *m_SwapchainImageViews[image_index],
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        {},
        {},
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        clear_color
    };

    vk::ClearValue depth_clear_value{ vk::ClearDepthStencilValue{ 1.0f, 0 } };
    vk::RenderingAttachmentInfo depth_attachment_info{
        *m_DepthImageView,
        vk::ImageLayout::eDepthStencilAttachmentOptimal,
        {},
        {},
        {},
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        depth_clear_value
    };

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

    vk::RenderingInfo rendering_info{
        vk::RenderingFlags{},
        {{ 0, 0 }, m_SwapchainExtent },
        1,
        0,
        1,
        &color_attachment_info,
        &depth_attachment_info,
        nullptr
    };

    command_buffer.beginRendering(rendering_info);

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_GraphicsPipeline);
    command_buffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_SwapchainExtent.width), static_cast<float>(m_SwapchainExtent.height), 0.0f, 1.0f));
    command_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_SwapchainExtent));

    command_buffer.bindVertexBuffers(0, *m_VertexBuffer, { 0 });
    command_buffer.bindIndexBuffer(*m_IndexBuffer, 0, vk::IndexType::eUint32);

    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipelineLayout, 0, *m_DescriptorSets[m_CurrentFrame], nullptr);
    command_buffer.draw(m_Vertices.size(), 1, 0, 0);

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
