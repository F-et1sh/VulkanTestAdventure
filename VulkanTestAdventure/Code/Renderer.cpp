#include "pch.h"
#include "Renderer.h"

void VKTest::Renderer::CreateRenderPass() {
    auto swapchain_format = m_SwapchainManager.getFormat();

    vk::AttachmentDescription color_attachment{
        vk::AttachmentDescriptionFlags{},   // Flags
        swapchain_format,                   // Swapchain Image Format
        vk::SampleCountFlagBits::e1,        // Samples
        vk::AttachmentLoadOp::eClear,       // LoadOp
        vk::AttachmentStoreOp::eStore,      // StoreOp
        vk::AttachmentLoadOp::eDontCare,    // Stencil LoadOp
        vk::AttachmentStoreOp::eDontCare,   // Stencil StoreOp
        vk::ImageLayout::eUndefined,        // Initial Layout
        vk::ImageLayout::ePresentSrcKHR     // Final Layout
    };

    vk::AttachmentReference color_attachment_ref{
        0,                                          // Attachment
        vk::ImageLayout::eColorAttachmentOptimal    // Layout
    };

    vk::SubpassDescription subpass{
        vk::SubpassDescriptionFlags{},      // Flags
        vk::PipelineBindPoint::eGraphics,   // Pipeline Bind Point
        0,                                  // Input Attachment Count
        nullptr,                            // Input Attachments
        1,                                  // Color Attachment Count
        &color_attachment_ref               // Color Attachments
    };

    vk::SubpassDependency dependency{
        VK_SUBPASS_EXTERNAL,                                // Source Subpass
        0,                                                  // Destination Subpass
        vk::PipelineStageFlagBits::eColorAttachmentOutput,  // Source Stage Mask
        vk::PipelineStageFlagBits::eColorAttachmentOutput,  // Destination Stage Mask
        {},                                                 // Source Access Mask
        vk::AccessFlagBits::eColorAttachmentWrite           // Destination Access Mask
    };

    vk::RenderPassCreateInfo render_pass_info{
        vk::RenderPassCreateFlags{},    // Flags
        1,                              // Color Attachment Count
        &color_attachment,              // Color Attachments
        1,                              // Subpass Count
        &subpass,                       // Subpasses
        1,                              // Dependency Count
        &dependency                     // Dependencies
    };

    auto& device = m_DeviceManager.getDevice();
    m_RenderPass = vk::raii::RenderPass{ device, render_pass_info };
}

void VKTest::Renderer::CreateGraphicsPipeline() {
    auto vert_shader_code = this->readFile("C:/Users/Пользователь/Desktop/VulkanTestAdventure/Files/Shaders/Test1/shader.vert.spv");
    auto frag_shader_code = this->readFile("C:/Users/Пользователь/Desktop/VulkanTestAdventure/Files/Shaders/Test1/shader.frag.spv");

    vk::raii::ShaderModule vert_shader_module{ this->createShaderModule(vert_shader_code) };
    vk::raii::ShaderModule frag_shader_module{ this->createShaderModule(frag_shader_code) };

    vk::PipelineShaderStageCreateInfo vert_shader_stage_info{
        vk::PipelineShaderStageCreateFlags{},   // Flags
        vk::ShaderStageFlagBits::eVertex,       // Stage
        *vert_shader_module,                    // Module
        "main"                                  // Name ( name of entry point )
    };

    vk::PipelineShaderStageCreateInfo frag_shader_stage_info{
        vk::PipelineShaderStageCreateFlags{},   // Flags
        vk::ShaderStageFlagBits::eFragment,     // Stage
        *frag_shader_module,                    // Module
        "main"                                  // Name ( name of entry point )
    };

    std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{ vert_shader_stage_info, frag_shader_stage_info };

    constexpr static auto binding_description{ Vertex::getBindingDescription() };
    constexpr static auto attribute_descriptions{ Vertex::getAttributeDescriptions() };

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{
        vk::PipelineVertexInputStateCreateFlags{},            // Flags
        1,                                                    // Binding description count
        &binding_description,                                 // pVertexBindingDescriptions
        static_cast<uint32_t>(attribute_descriptions.size()), // Attribute description count
        attribute_descriptions.data()                         // pVertexAttributeDescriptions
    };

    vk::PipelineInputAssemblyStateCreateInfo input_assembly{
        vk::PipelineInputAssemblyStateCreateFlags{}, // Flags
        vk::PrimitiveTopology::eTriangleList,        // Topology
        vk::False                                    // Primitive restart
    };

    vk::PipelineViewportStateCreateInfo viewport_state{
        vk::PipelineViewportStateCreateFlags{}, // Flags
        1,                                      // Viewport count
        nullptr,                                // pViewports
        1,                                      // Scissor count
        nullptr                                 // pScissors
    };

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        vk::PipelineRasterizationStateCreateFlags{}, // Flags
        vk::False,                                   // Depth clamp
        vk::False,                                   // Rasterizer discard
        vk::PolygonMode::eFill,                      // Polygon mode
        vk::CullModeFlagBits::eBack,                 // Cull mode
        vk::FrontFace::eCounterClockwise,            // Front face
        vk::False,                                   // Depth bias enable
        0.0f,                                        // Depth bias constant factor
        0.0f,                                        // Depth bias clamp
        0.0f,                                        // Depth bias slope factor
        1.0f                                         // Line width
    };

    vk::PipelineMultisampleStateCreateInfo multisampling{
        vk::PipelineMultisampleStateCreateFlags{}, // Flags
        m_MSAA_Samples,                            // Rasterization samples
        vk::False,                                 // Sample shading enable
        1.0f,                                      // Min sample shading
        nullptr,                                   // pSampleMask
        vk::False,                                 // Alpha to coverage enable
        vk::False                                  // Alpha to one enable
    };

    vk::PipelineDepthStencilStateCreateInfo depth_stencil{
        vk::PipelineDepthStencilStateCreateFlags{}, // Flags
        vk::True,                                   // Depth test enable
        vk::True,                                   // Depth write enable
        vk::CompareOp::eLess,                       // Depth compare op
        vk::False,                                  // Depth bounds test enable
        vk::False,                                  // Stencil test enable
        {},                                         // Front
        {},                                         // Back
        0.0f,                                       // Min depth bounds
        1.0f                                        // Max depth bounds
    };

    vk::PipelineColorBlendAttachmentState color_blend_attachment{
        vk::True, // Blend enable
        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo color_blending{
        vk::PipelineColorBlendStateCreateFlags{}, // Flags
        vk::False,                                // Logic op enable
        vk::LogicOp::eCopy,                       // Logic op
        1,                                        // Attachment count
        &color_blend_attachment,                  // pAttachments
        { 0.0f, 0.0f, 0.0f, 0.0f }                // Blend constants
    };

    std::vector<vk::DynamicState> dynamic_states{
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{
        vk::PipelineDynamicStateCreateFlags{},        // Flags
        static_cast<uint32_t>(dynamic_states.size()), // Dynamic state count
        dynamic_states.data()                         // pDynamicStates
    };

    vk::PipelineLayoutCreateInfo pipeline_layout_info{
        vk::PipelineLayoutCreateFlags{}, // Flags
        1,                               // Set layout count
        &*m_DescriptorSetLayout,         // pSetLayouts
        0,                               // Push constant range count
        nullptr                          // pPushConstantRanges
    };

    auto& device = m_DeviceManager.getDevice();
    m_PipelineLayout = vk::raii::PipelineLayout{ device, pipeline_layout_info };

    vk::GraphicsPipelineCreateInfo pipeline_info{
        vk::PipelineCreateFlags{},                   // Flags
        static_cast<uint32_t>(shader_stages.size()), // Stage count
        shader_stages.data(),                        // pStages
        &vertex_input_info,                          // pVertexInputState
        &input_assembly,                             // pInputAssemblyState
        nullptr,                                     // pTessellationState
        &viewport_state,                             // pViewportState
        &rasterizer,                                 // pRasterizationState
        &multisampling,                              // pMultisampleState
        &depth_stencil,                              // pDepthStencilState
        &color_blending,                             // pColorBlendState
        &dynamic_state,                              // pDynamicState
        *m_PipelineLayout,                           // Layout
        *m_RenderPass,                               // Render pass
        0,                                           // Subpass
        nullptr,                                     // Base pipeline handle
        -1                                           // Base pipeline index
    };

    m_GraphicsPipeline = vk::raii::Pipeline{ device, nullptr, pipeline_info };
}

std::vector<char> VKTest::Renderer::readFile(const std::filesystem::path& path) {
    std::ifstream file{ path, std::ios::ate | std::ios::binary };
    
    if (!file.good())
        RUNTIME_ERROR("Failed to read file\nPath : " + path.string());
    
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

vk::raii::ShaderModule VKTest::Renderer::createShaderModule(const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo create_info{
        vk::ShaderModuleCreateFlags{},                  // Flags
        code.size(),                                    // Code Size
        reinterpret_cast<const uint32_t*>(code.data()), // Code
    };

    const auto& device = m_DeviceManager.getDevice();
    return device.createShaderModule(create_info);
}
