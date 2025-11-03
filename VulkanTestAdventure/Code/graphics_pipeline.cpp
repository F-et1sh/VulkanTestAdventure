/*
* Copyright (c) 2025, NVIDIA CORPORATION.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* SPDX-FileCopyrightText: Copyright (c) 2025, NVIDIA CORPORATION.
* SPDX-License-Identifier: Apache-2.0
*/

/*
* Modified by Farrakh
* 2025
*/

#include "pch.h"
#include "graphics_pipeline.hpp"

namespace vk_test {

    void GraphicsPipelineState::cmdApplyAllStates(VkCommandBuffer cmd) const {
        vkCmdSetLineWidth(cmd, rasterizationState.lineWidth);
        vkCmdSetLineStippleEnableEXT(cmd, rasterizationLineState.stippledLineEnable);
        vkCmdSetLineRasterizationModeEXT(cmd, rasterizationLineState.lineRasterizationMode);
        if (rasterizationLineState.stippledLineEnable != 0u) {
            vkCmdSetLineStipple(cmd, rasterizationLineState.lineStippleFactor, rasterizationLineState.lineStipplePattern);
        }

        vkCmdSetRasterizerDiscardEnable(cmd, rasterizationState.rasterizerDiscardEnable);
        vkCmdSetPolygonModeEXT(cmd, rasterizationState.polygonMode);
        vkCmdSetCullMode(cmd, rasterizationState.cullMode);
        vkCmdSetFrontFace(cmd, rasterizationState.frontFace);
        vkCmdSetDepthBiasEnable(cmd, rasterizationState.depthBiasEnable);
        if (rasterizationState.depthBiasEnable != 0u) {
            vkCmdSetDepthBias(cmd, rasterizationState.depthBiasConstantFactor, rasterizationState.depthBiasClamp, rasterizationState.depthBiasSlopeFactor);
        }
        vkCmdSetDepthClampEnableEXT(cmd, rasterizationState.depthClampEnable);

        vkCmdSetDepthTestEnable(cmd, depthStencilState.depthTestEnable);
        if (depthStencilState.depthTestEnable != 0u) {
            vkCmdSetDepthBounds(cmd, depthStencilState.minDepthBounds, depthStencilState.maxDepthBounds);
            vkCmdSetDepthBoundsTestEnable(cmd, depthStencilState.depthBoundsTestEnable);
            vkCmdSetDepthCompareOp(cmd, depthStencilState.depthCompareOp);
            vkCmdSetDepthWriteEnable(cmd, depthStencilState.depthWriteEnable);
        }

        vkCmdSetStencilTestEnable(cmd, depthStencilState.stencilTestEnable);
        if (depthStencilState.stencilTestEnable != 0u) {
            vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FACE_FRONT_BIT, depthStencilState.front.compareMask);
            vkCmdSetStencilCompareMask(cmd, VK_STENCIL_FACE_BACK_BIT, depthStencilState.back.compareMask);
            vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FACE_FRONT_BIT, depthStencilState.front.writeMask);
            vkCmdSetStencilWriteMask(cmd, VK_STENCIL_FACE_BACK_BIT, depthStencilState.back.writeMask);
            vkCmdSetStencilReference(cmd, VK_STENCIL_FACE_FRONT_BIT, depthStencilState.front.reference);
            vkCmdSetStencilReference(cmd, VK_STENCIL_FACE_BACK_BIT, depthStencilState.back.reference);
            vkCmdSetStencilOp(cmd, VK_STENCIL_FACE_FRONT_BIT, depthStencilState.front.failOp, depthStencilState.front.passOp, depthStencilState.front.depthFailOp, depthStencilState.front.compareOp);
            vkCmdSetStencilOp(cmd, VK_STENCIL_FACE_BACK_BIT, depthStencilState.back.failOp, depthStencilState.back.passOp, depthStencilState.back.depthFailOp, depthStencilState.back.compareOp);
        }

        vkCmdSetPrimitiveRestartEnable(cmd, inputAssemblyState.primitiveRestartEnable);
        vkCmdSetPrimitiveTopology(cmd, inputAssemblyState.topology);

        vkCmdSetRasterizationSamplesEXT(cmd, multisampleState.rasterizationSamples);
        vkCmdSetSampleMaskEXT(cmd, multisampleState.rasterizationSamples, &sampleMask);
        vkCmdSetAlphaToCoverageEnableEXT(cmd, multisampleState.alphaToCoverageEnable);
        vkCmdSetAlphaToOneEnableEXT(cmd, multisampleState.alphaToOneEnable);

        if ((!vertexBindings.empty() != 0u) && (!vertexAttributes.empty() != 0u)) {
            vkCmdSetVertexInputEXT(cmd, static_cast<uint32_t>(vertexBindings.size()), vertexBindings.data(), static_cast<uint32_t>(vertexAttributes.size()), vertexAttributes.data());
        }

        assert(colorWriteMasks.size() == colorBlendEquations.size() && colorWriteMasks.size() == colorBlendEnables.size());

        uint32_t attachment_count = static_cast<uint32_t>(colorWriteMasks.size());
        if (attachment_count != 0u) {
            vkCmdSetColorBlendEquationEXT(cmd, 0, attachment_count, colorBlendEquations.data());
            vkCmdSetColorBlendEnableEXT(cmd, 0, attachment_count, colorBlendEnables.data());
            vkCmdSetColorWriteMaskEXT(cmd, 0, attachment_count, colorWriteMasks.data());
        }

        vkCmdSetBlendConstants(cmd, colorBlendState.blendConstants);
        vkCmdSetLogicOpEnableEXT(cmd, colorBlendState.logicOpEnable);
    }

    void GraphicsPipelineState::cmdApplyDynamicStates(VkCommandBuffer cmd, std::span<const VkDynamicState> dynamic_states) const {
        for (VkDynamicState state : dynamic_states) {
            cmdApplyDynamicState(cmd, state);
        }
    }

    //////////////////////////////////////////////////////////////////////////

    void GraphicsPipelineCreator::clearShaders() {
        m_ShaderStages.clear();
        m_ShaderStageModules.clear();
        m_ShaderStageSubgroupSizes.clear();
    }

    void GraphicsPipelineCreator::addShader(VkShaderStageFlagBits       stage,
                                            const char*                 p_entry_name,
                                            size_t                      spirv_size_in_bytes,
                                            const uint32_t*             spirv_data,
                                            const VkSpecializationInfo* p_specialization_info /*= nullptr*/,
                                            uint32_t                    subgroup_required_size /*= 0*/) {
        VkPipelineShaderStageCreateInfo shader_info = {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage               = stage,
            .pName               = p_entry_name,
            .pSpecializationInfo = p_specialization_info,
        };

        VkShaderModuleCreateInfo module_info = {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spirv_size_in_bytes,
            .pCode    = spirv_data,
        };

        VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup_info = {
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO,
            .requiredSubgroupSize = subgroup_required_size,
        };

        m_ShaderStages.emplace_back(shader_info);
        m_ShaderStageModules.emplace_back(module_info);
        m_ShaderStageSubgroupSizes.emplace_back(subgroup_info);
    }

    void GraphicsPipelineCreator::addShader(VkShaderStageFlagBits       stage,
                                            const char*                 p_entry_name,
                                            VkShaderModule              shader_module,
                                            const VkSpecializationInfo* p_specialization_info /*= nullptr*/,
                                            uint32_t                    subgroup_required_size /*= 0*/) {
        VkPipelineShaderStageCreateInfo shader_info = {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage               = stage,
            .module              = shader_module,
            .pName               = p_entry_name,
            .pSpecializationInfo = p_specialization_info,
        };

        VkPipelineShaderStageRequiredSubgroupSizeCreateInfo subgroup_info = {
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO,
            .requiredSubgroupSize = subgroup_required_size,
        };

        m_ShaderStages.emplace_back(shader_info);
        m_ShaderStageModules.push_back({});
        m_ShaderStageSubgroupSizes.emplace_back(subgroup_info);
    }

    void GraphicsPipelineCreator::addShader(VkShaderStageFlagBits       stage,
                                            const char*                 p_entry_name,
                                            std::span<uint32_t const>   spirv_data,
                                            const VkSpecializationInfo* p_specialization_info /*= nullptr*/,
                                            uint32_t                    subgroup_required_size /*= 0*/) {
        addShader(stage, p_entry_name, spirv_data.size_bytes(), spirv_data.data(), p_specialization_info, subgroup_required_size);
    }

    VkResult GraphicsPipelineCreator::createGraphicsPipeline(VkDevice                     device,
                                                             VkPipelineCache              cache,
                                                             const GraphicsPipelineState& graphics_state,
                                                             VkPipeline*                  p_pipeline) {
        VkGraphicsPipelineCreateInfo pipeline_info_temp;

        buildPipelineCreateInfo(pipeline_info_temp, graphics_state);

        VkResult result = vkCreateGraphicsPipelines(device, cache, 1, &pipeline_info_temp, nullptr, p_pipeline);

        return result;
    }

    void GraphicsPipelineCreator::buildPipelineCreateInfo(VkGraphicsPipelineCreateInfo& create_info_temp, const GraphicsPipelineState& graphics_state) {
        // check unsupported input states
        assert(pipelineInfo.pColorBlendState == nullptr);
        assert(pipelineInfo.pDepthStencilState == nullptr);
        assert(pipelineInfo.pDynamicState == nullptr);
        assert(pipelineInfo.pInputAssemblyState == nullptr);
        assert(pipelineInfo.pMultisampleState == nullptr);
        assert(pipelineInfo.pRasterizationState == nullptr);
        assert(pipelineInfo.pTessellationState == nullptr);
        assert(pipelineInfo.pVertexInputState == nullptr);
        assert(pipelineInfo.pViewportState == nullptr);

        assert(graphics_state.rasterizationState.pNext == nullptr);
        assert(graphics_state.multisampleState.pSampleMask == nullptr);
        assert(graphics_state.tessellationState.pNext == nullptr);

        assert(graphics_state.vertexInputState.pVertexBindingDescriptions == nullptr);
        assert(graphics_state.vertexInputState.pVertexAttributeDescriptions == nullptr);
        assert(graphics_state.vertexInputState.vertexBindingDescriptionCount == 0);
        assert(graphics_state.vertexInputState.vertexAttributeDescriptionCount == 0);

        assert(graphics_state.colorBlendState.pAttachments == nullptr);
        assert(graphics_state.colorBlendState.attachmentCount == 0);

        assert(graphics_state.colorWriteMasks.size() == graphics_state.colorBlendEquations.size() && graphics_state.colorWriteMasks.size() == graphics_state.colorBlendEnables.size());

        // copy data that we end up modifying
        create_info_temp     = pipelineInfo;
        m_RasterizationState = graphics_state.rasterizationState;
        m_MultisampleState   = graphics_state.multisampleState;
        m_TessellationState  = graphics_state.tessellationState;
        m_VertexInputState   = graphics_state.vertexInputState;
        m_RenderingState     = renderingState;

        // setup various pointers
        if (pipelineInfo.renderPass == nullptr) {
            m_RenderingState.colorAttachmentCount    = static_cast<uint32_t>(colorFormats.size());
            m_RenderingState.pColorAttachmentFormats = colorFormats.data();

            m_RenderingState.pNext = create_info_temp.pNext;
            create_info_temp.pNext = &m_RenderingState;
        }

        if (flags2 != 0) {
            // Only valid to enqueue if flags are non-zero
            m_Flags2Info.flags = flags2;

            m_Flags2Info.pNext     = create_info_temp.pNext;
            create_info_temp.pNext = &m_Flags2Info;
        }

        create_info_temp.pColorBlendState    = &m_ColorBlendState;
        create_info_temp.pDepthStencilState  = &graphics_state.depthStencilState;
        create_info_temp.pDynamicState       = &m_DynamicState;
        create_info_temp.pInputAssemblyState = &graphics_state.inputAssemblyState;
        create_info_temp.pMultisampleState   = &m_MultisampleState;
        create_info_temp.pRasterizationState = &m_RasterizationState;
        create_info_temp.pTessellationState  = &m_TessellationState;
        create_info_temp.pVertexInputState   = &m_VertexInputState;
        create_info_temp.pViewportState      = &viewportState;

        m_RasterizationState.pNext     = &graphics_state.rasterizationLineState;
        m_MultisampleState.pSampleMask = &graphics_state.sampleMask;
        m_TessellationState.pNext      = &graphics_state.tessellationDomainOriginState;

        // setup local arrays

        m_DynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateValues.size());
        m_DynamicState.pDynamicStates    = dynamicStateValues.data();

        m_StaticVertexBindings.resize(graphics_state.vertexBindings.size());
        m_StaticVertexBindingDivisors.resize(graphics_state.vertexBindings.size());
        m_StaticVertexAttributes.resize(graphics_state.vertexAttributes.size());

        m_VertexInputState.pVertexBindingDescriptions    = m_StaticVertexBindings.data();
        m_VertexInputState.pVertexAttributeDescriptions  = m_StaticVertexAttributes.data();
        m_VertexInputDivisorState.pVertexBindingDivisors = m_StaticVertexBindingDivisors.data();

        m_VertexInputState.vertexBindingDescriptionCount   = static_cast<uint32_t>(m_StaticVertexBindings.size());
        m_VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_StaticVertexAttributes.size());

        // binding divisors are
        uint32_t divisor_count = 0;

        m_VertexInputDivisorState.vertexBindingDivisorCount = 0;
        m_VertexInputDivisorState.pNext                     = graphics_state.vertexInputState.pNext;

        for (size_t i = 0; i < graphics_state.vertexBindings.size(); i++) {
            m_StaticVertexBindings[i].binding   = graphics_state.vertexBindings[i].binding;
            m_StaticVertexBindings[i].inputRate = graphics_state.vertexBindings[i].inputRate;
            m_StaticVertexBindings[i].stride    = graphics_state.vertexBindings[i].stride;
            if (m_StaticVertexBindings[i].inputRate != VK_VERTEX_INPUT_RATE_VERTEX) {
                m_StaticVertexBindingDivisors[divisor_count].binding = graphics_state.vertexBindings[i].binding;
                m_StaticVertexBindingDivisors[divisor_count].divisor = graphics_state.vertexBindings[i].divisor;
                divisor_count++;
            }
        }

        if (divisor_count != 0u) {
            m_VertexInputDivisorState.vertexBindingDivisorCount = divisor_count;

            m_VertexInputDivisorState.pNext = graphics_state.vertexInputState.pNext;
            m_VertexInputState.pNext        = &m_VertexInputDivisorState;
        }

        for (size_t i = 0; i < graphics_state.vertexAttributes.size(); i++) {
            m_StaticVertexAttributes[i].binding  = graphics_state.vertexAttributes[i].binding;
            m_StaticVertexAttributes[i].format   = graphics_state.vertexAttributes[i].format;
            m_StaticVertexAttributes[i].location = graphics_state.vertexAttributes[i].location;
            m_StaticVertexAttributes[i].offset   = graphics_state.vertexAttributes[i].offset;
        }

        m_StaticAttachmentState.resize(graphics_state.colorWriteMasks.size());

        m_ColorBlendState.attachmentCount = static_cast<uint32_t>(m_StaticAttachmentState.size());
        m_ColorBlendState.pAttachments    = m_StaticAttachmentState.data();

        for (size_t i = 0; i < graphics_state.colorWriteMasks.size(); i++) {
            m_StaticAttachmentState[i].blendEnable         = graphics_state.colorBlendEnables[i];
            m_StaticAttachmentState[i].colorWriteMask      = graphics_state.colorWriteMasks[i];
            m_StaticAttachmentState[i].alphaBlendOp        = graphics_state.colorBlendEquations[i].alphaBlendOp;
            m_StaticAttachmentState[i].colorBlendOp        = graphics_state.colorBlendEquations[i].colorBlendOp;
            m_StaticAttachmentState[i].dstAlphaBlendFactor = graphics_state.colorBlendEquations[i].dstAlphaBlendFactor;
            m_StaticAttachmentState[i].dstColorBlendFactor = graphics_state.colorBlendEquations[i].dstColorBlendFactor;
            m_StaticAttachmentState[i].srcAlphaBlendFactor = graphics_state.colorBlendEquations[i].srcAlphaBlendFactor;
            m_StaticAttachmentState[i].srcColorBlendFactor = graphics_state.colorBlendEquations[i].srcColorBlendFactor;
        }

        if (!m_ShaderStages.empty() != 0u) {
            // if we use locally provided shaders, then none must have been provided otherwise
            assert(create_info_temp.stageCount == 0 && create_info_temp.pStages == nullptr);

            create_info_temp.stageCount = static_cast<uint32_t>(m_ShaderStages.size());
            create_info_temp.pStages    = m_ShaderStages.data();

            for (uint32_t i = 0; i < create_info_temp.stageCount; i++) {
                if (m_ShaderStages[i].module == nullptr) {
                    m_ShaderStages[i].pNext = &m_ShaderStageModules[i];
                }
                if (m_ShaderStageSubgroupSizes[i].requiredSubgroupSize != 0u) {
                    m_ShaderStageSubgroupSizes[i].pNext = const_cast<void*>(m_ShaderStages[i].pNext);
                    m_ShaderStages[i].pNext             = &m_ShaderStageSubgroupSizes[i];
                }
            }
        }
    }

} // namespace vk_test

//--------------------------------------------------------------------------------------------------
// Usage example
//--------------------------------------------------------------------------------------------------
static void usage_GraphicsPipeline() {
    VkDevice device{};

    vk_test::GraphicsPipelineState graphicsState;

    // set some state
    // we are omitting most things to keep it short
    graphicsState.depthStencilState.depthTestEnable  = VK_TRUE;
    graphicsState.depthStencilState.depthWriteEnable = VK_TRUE;
    graphicsState.depthStencilState.depthCompareOp   = VK_COMPARE_OP_GREATER_OR_EQUAL;

    // example using traditional pipeline
    {
        std::vector<uint32_t> vertex_code;
        std::vector<uint32_t> fragment_code;

        // we want to create a traditional pipeline
        vk_test::GraphicsPipelineCreator graphics_pipeline_creator;

        // manipulate the public members of the class directly to change the state used for creation
        graphics_pipeline_creator.flags2 = VK_PIPELINE_CREATE_2_CAPTURE_STATISTICS_BIT_KHR;

        graphics_pipeline_creator.dynamicStateValues.push_back(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE);
        graphics_pipeline_creator.dynamicStateValues.push_back(VK_DYNAMIC_STATE_DEPTH_COMPARE_OP);

        graphics_pipeline_creator.addShader(VK_SHADER_STAGE_VERTEX_BIT, "main", vertex_code.size() * sizeof(uint32_t), vertex_code.data());
        graphics_pipeline_creator.addShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", fragment_code.size() * sizeof(uint32_t), fragment_code.data());

        // create the actual pipeline from a combination of state within `graphicsPipelineCreator` and `graphicsState`
        VkPipeline graphics_pipeline = nullptr;
        VkResult   result            = graphics_pipeline_creator.createGraphicsPipeline(device, nullptr, graphicsState, &graphics_pipeline);

        VkCommandBuffer cmd{};
        VkExtent2D      viewport_size{};

        //we recommend (and set defaults) to always use dynamic state for viewport and scissor
        vk_test::GraphicsPipelineState::cmdSetViewportAndScissor(cmd, viewport_size);

        // bind pipeline
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

        vkCmdSetDepthCompareOp(cmd, VK_COMPARE_OP_GREATER_OR_EQUAL);
        vkCmdSetDepthWriteEnable(cmd, VK_TRUE);

        vkCmdDraw(cmd, 1, 2, 3, 4);

        vkCmdSetDepthCompareOp(cmd, VK_COMPARE_OP_EQUAL);
        vkCmdSetDepthWriteEnable(cmd, VK_FALSE);

        vkCmdDraw(cmd, 1, 2, 3, 4);
    }

    // example in combination with shader objects
    {
        VkCommandBuffer cmd{};
        VkExtent2D      viewport_size{};

        VkShaderEXT vertex_shader{};
        VkShaderEXT fragment_shader{};

        // note this is actually the static function as before, but for looks we used the graphicsState
        vk_test::GraphicsPipelineState::cmdSetViewportAndScissor(cmd, viewport_size);

        // bind default state via struct
        graphicsState.cmdApplyAllStates(cmd);

        // bind the shaders
        vk_test::GraphicsPipelineState::BindableShaders bindable_shaders;
        bindable_shaders.vertex   = vertex_shader;
        bindable_shaders.fragment = fragment_shader;

        bool supports_mesh_shaders = true;

        // also a static function
        vk_test::GraphicsPipelineState::cmdBindShaders(cmd, bindable_shaders, supports_mesh_shaders);

        vkCmdSetDepthCompareOp(cmd, VK_COMPARE_OP_GREATER_OR_EQUAL);
        vkCmdSetDepthWriteEnable(cmd, VK_TRUE);

        vkCmdDraw(cmd, 1, 2, 3, 4);

        vkCmdSetDepthCompareOp(cmd, VK_COMPARE_OP_EQUAL);
        vkCmdSetDepthWriteEnable(cmd, VK_FALSE);

        vkCmdDraw(cmd, 1, 2, 3, 4);
    }
}
