#include "pch.h"
#include "PipelineManager.h"

VKTest::PipelineManager::PipelineManager(DeviceManager* device_manager, RenderPassManager* render_pass_manager, GPUResourceManager* gpu_resource_manager, SwapchainManager* swapchain_manager) :
    p_DeviceManager{ device_manager }, p_RenderPassManager{ render_pass_manager }, p_GPUResourceManager{ gpu_resource_manager }, p_SwapchainManager{ swapchain_manager } {}

void VKTest::PipelineManager::CreateGraphicsPipeline() {
	auto vert_shader_code = this->readFile(L"C:/Users/Пользователь/Desktop/VulkanTestAdventure/Files/Shaders/Test1/shader.vert.spv");
    auto frag_shader_code = this->readFile(L"C:/Users/Пользователь/Desktop/VulkanTestAdventure/Files/Shaders/Test1/shader.frag.spv");

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
        p_RenderPassManager->getMSAASamples(),     // Rasterization samples
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
        vk::PipelineLayoutCreateFlags{},                  // Flags
        1,                                                // Set layout count
        &*p_GPUResourceManager->getDescriptorSetLayout(), // pSetLayouts
        0,                                                // Push constant range count
        nullptr                                           // pPushConstantRanges
    };

    auto& device = p_DeviceManager->getDevice();
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
        *p_RenderPassManager->getRenderPass(),       // Render pass
        0,                                           // Subpass
        nullptr,                                     // Base pipeline handle
        -1                                           // Base pipeline index
    };

    m_GraphicsPipeline = vk::raii::Pipeline{ device, nullptr, pipeline_info };
}

void VKTest::PipelineManager::recordCommandBuffer(vk::raii::CommandBuffer& command_buffer, uint32_t image_index) {
    vk::CommandBufferBeginInfo begin_info{};

    command_buffer.begin(begin_info);

    static constexpr std::array<vk::ClearValue, 2> clear_values{
        vk::ClearValue{ vk::ClearColorValue{ 0.1f, 0.1f, 0.1f, 1.0f } }, // Color
        vk::ClearValue{ vk::ClearDepthStencilValue{ 1.0f, 0.0f } }       // Depth and Stencil
    };

    auto& framebuffer = p_SwapchainManager->getFramebuffers()[image_index];

    vk::RenderPassBeginInfo render_pass_info{
        p_RenderPassManager->getRenderPass(),               // Render Pass
        framebuffer,                                        // Framebuffer
        vk::Rect2D{                                         // Render Area
            vk::Offset2D{ 0, 0 },                           // Offset
            vk::Extent2D{ p_SwapchainManager->getExtent() } // Extent
        },
        clear_values
    };

    command_buffer.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);

    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_GraphicsPipeline);

    vk::Viewport viewport{
        0.0f,                                                       // x
        0.0f,                                                       // y
        static_cast<float>(p_SwapchainManager->getExtent().width),  // width
        static_cast<float>(p_SwapchainManager->getExtent().height), // height
        0.0f,                                                       // Min Depth
        1.0f                                                        // Max Depth
    };

    command_buffer.setViewport(0, viewport);

    vk::Rect2D scissor{
        vk::Offset2D{ 0, 0 },                           // Offset
        vk::Extent2D{ p_SwapchainManager->getExtent() } // Extent
    };

    command_buffer.setScissor(0, scissor);

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vk::Buffer vertex_buffer[] = { VERTICES.data() };

    // Draw each object with its own descriptor set
    for (const auto& gameObject : gameObjects) {
        // Bind the descriptor set for this object
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, gameObject.descriptorSets.size(), &gameObject.descriptorSets[currentFrame], 0, nullptr);

        // Draw the object
        vkCmdDrawIndexed(command_buffer, indices.size(), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

std::vector<char> VKTest::PipelineManager::readFile(const std::filesystem::path& path) {
    std::ifstream file{ path, std::ios::ate | std::ios::binary };
    if (!file.good()) RUNTIME_ERROR("ERROR : Failed to open file\nPath : " + path.string());

    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    return buffer;
}

vk::raii::ShaderModule VKTest::PipelineManager::createShaderModule(const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo create_info{
        vk::ShaderModuleCreateFlags{},                  // Flags
        code.size(),                                    // Code Size
        reinterpret_cast<const uint32_t*>(code.data()), // Code
    };

    const auto& device = p_DeviceManager->getDevice();
    return device.createShaderModule(create_info);
}
