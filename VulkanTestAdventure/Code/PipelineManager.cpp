#include "pch.h"
#include "PipelineManager.h"

void VKTest::PipelineManager::Release() {
    vkDestroyPipeline(p_DeviceManager->getDevice(), m_GraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(p_DeviceManager->getDevice(), m_PipelineLayout, nullptr);

    vkDestroyDescriptorPool(p_DeviceManager->getDevice(), m_DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(p_DeviceManager->getDevice(), m_DescriptorSetLayout, nullptr);
}

void VKTest::PipelineManager::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding            = 0;
    ubo_layout_binding.descriptorCount    = 1;
    ubo_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.pImmutableSamplers = nullptr;
    ubo_layout_binding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding sampler_layout_binding{};
    sampler_layout_binding.binding            = 1;
    sampler_layout_binding.descriptorCount    = 1;
    sampler_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };
    VkDescriptorSetLayoutCreateInfo             layout_info{};
    layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.pBindings    = bindings.data();

    if (vkCreateDescriptorSetLayout(p_DeviceManager->getDevice(), &layout_info, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to create descriptor set layout");
    }
}

void VKTest::PipelineManager::CreateGraphicsPipeline() {
    auto vert_shader_code = readFile("F:/Windows/Desktop/VulkanTestAdventure/Files/Shaders/Test1/shader.vert.spv");
    auto frag_shader_code = readFile("F:/Windows/Desktop/VulkanTestAdventure/Files/Shaders/Test1/shader.frag.spv");

    VkShaderModule vert_shader_module = createShaderModule(vert_shader_code);
    VkShaderModule frag_shader_module = createShaderModule(frag_shader_code);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName  = "main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName  = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto binding_description    = Vertex::getBindingDescription();
    auto attribute_descriptions = Vertex::getAttributeDescriptions();

    vertex_input_info.vertexBindingDescriptionCount   = 1;
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexBindingDescriptions      = &binding_description;
    vertex_input_info.pVertexAttributeDescriptions    = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0F;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = p_DeviceManager->getMSAASamples();

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable       = VK_TRUE;
    depth_stencil.depthWriteEnable      = VK_TRUE;
    depth_stencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable     = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable     = VK_FALSE;
    color_blending.logicOp           = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount   = 1;
    color_blending.pAttachments      = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0F;
    color_blending.blendConstants[1] = 0.0F;
    color_blending.blendConstants[2] = 0.0F;
    color_blending.blendConstants[3] = 0.0F;

    std::vector<VkDynamicState> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates    = dynamic_states.data();

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1; // idk why there was 2, i put here 1
    pipeline_layout_info.pSetLayouts    = &m_DescriptorSetLayout;

    if (vkCreatePipelineLayout(p_DeviceManager->getDevice(), &pipeline_layout_info, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount          = 2;
    pipeline_info.pStages             = shader_stages;
    pipeline_info.pVertexInputState   = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState      = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState   = &multisampling;
    pipeline_info.pDepthStencilState  = &depth_stencil;
    pipeline_info.pColorBlendState    = &color_blending;
    pipeline_info.pDynamicState       = &dynamic_state;
    pipeline_info.layout              = m_PipelineLayout;
    pipeline_info.renderPass          = p_RenderPassManager->getRenderPass();
    pipeline_info.subpass             = 0;
    pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(p_DeviceManager->getDevice(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to create graphics pipeline");
    }

    vkDestroyShaderModule(p_DeviceManager->getDevice(), frag_shader_module, nullptr);
    vkDestroyShaderModule(p_DeviceManager->getDevice(), vert_shader_module, nullptr);
}

void VKTest::PipelineManager::CreateDescriptorPool() {
    // We need MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT descriptor sets
    std::array pool_size{
        VkDescriptorPoolSize(VkDescriptorType::VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, RenderMesh::MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT),
        VkDescriptorPoolSize(VkDescriptorType::VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, RenderMesh::MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT)
    };
    VkDescriptorPoolCreateInfo pool_info{
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = VkDescriptorPoolCreateFlagBits::VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets       = RenderMesh::MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(pool_size.size()),
        .pPoolSizes    = pool_size.data()
    };
    vkCreateDescriptorPool(p_DeviceManager->getDevice(), &pool_info, nullptr, &m_DescriptorPool);
}

void VKTest::PipelineManager::CreateDescriptorSets() {
    // For each game object
    for (auto& game_object : p_RenderMesh->getGameObjects()) {
        // Create descriptor sets for each frame in flight
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_DescriptorSetLayout);

        VkDescriptorSetAllocateInfo alloc_info{
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = m_DescriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts        = layouts.data()
        };

        game_object.descriptor_sets.clear();
        game_object.descriptor_sets.resize(layouts.size());
        vkAllocateDescriptorSets(p_DeviceManager->getDevice(), &alloc_info, game_object.descriptor_sets.data());

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo buffer_info{
                .buffer = game_object.uniform_buffers[i],
                .offset = 0,
                .range  = sizeof(UniformBufferObject)
            };

            VkDescriptorImageInfo image_info{
                .sampler     = p_RenderMesh->getTextureSampler(),
                .imageView   = p_RenderMesh->getTextureImageView(),
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            std::array descriptor_writes{
                VkWriteDescriptorSet{
                    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet          = game_object.descriptor_sets[i],
                    .dstBinding      = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo     = &buffer_info },
                VkWriteDescriptorSet{
                    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet          = game_object.descriptor_sets[i],
                    .dstBinding      = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo      = &image_info }
            };

            vkUpdateDescriptorSets(p_DeviceManager->getDevice(), descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
        }
    }
}

std::vector<char> VKTest::PipelineManager::readFile(const std::filesystem::path& path) {
    std::ifstream file{ path, std::ios::ate | std::ios::binary };
    if (!file.good()) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to open file\nPath : " + path.string());
    }

    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    return buffer;
}

VkShaderModule VKTest::PipelineManager::createShaderModule(const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo create_info{
        vk::ShaderModuleCreateFlags{},                  // Flags
        code.size(),                                    // Code Size
        reinterpret_cast<const uint32_t*>(code.data()), // Code
    };

    VkShaderModule shader_module{};
    vkCreateShaderModule(p_DeviceManager->getDevice(), create_info, nullptr, &shader_module);
    return shader_module;
}
