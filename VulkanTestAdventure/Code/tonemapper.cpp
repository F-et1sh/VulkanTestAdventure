/*
 * Copyright (c) 2023-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2023-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pch.h"
#include "tonemapper.hpp"

#include <barriers.hpp>
#include <commands.hpp>
#include <compute_pipeline.hpp>

#include "../../Files/Shaders/tonemap_functions.h.slang"

VkResult vk_test::Tonemapper::init(vk_test::ResourceAllocator* alloc, std::span<const uint32_t> spirv) {
    assert(!m_Device);
    m_Alloc  = alloc;
    m_Device = alloc->getDevice();

    // Create buffers
    alloc->createBuffer(m_ExposureBuffer, sizeof(float), VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO);
    alloc->createBuffer(m_HistogramBuffer, sizeof(uint32_t) * EXPOSURE_HISTOGRAM_SIZE, VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO);

    // Shader descriptor set layout
    vk_test::DescriptorBindings bindings;
    bindings.addBinding(shaderio::TonemapBinding::eImageInput, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
    bindings.addBinding(shaderio::TonemapBinding::eImageOutput, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT);
    bindings.addBinding(shaderio::TonemapBinding::eHistogramInputOutput, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);
    bindings.addBinding(shaderio::TonemapBinding::eLuminanceInputOutput, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    m_DescriptorPack.init(bindings, m_Device, 0, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);

    // Push constant
    VkPushConstantRange push_constant_range{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .size       = sizeof(shaderio::TonemapperData)
    };

    // Pipeline layout
    const VkPipelineLayoutCreateInfo pipeline_layout_info{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 1,
        .pSetLayouts            = m_DescriptorPack.getLayoutPtr(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &push_constant_range,
    };
    vkCreatePipelineLayout(m_Device, &pipeline_layout_info, nullptr, &m_PipelineLayout);

    // Compute Pipeline
    VkComputePipelineCreateInfo comp_info   = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    VkShaderModuleCreateInfo    shader_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    comp_info.stage                         = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    comp_info.stage.stage                   = VK_SHADER_STAGE_COMPUTE_BIT;
    comp_info.stage.pNext                   = &shader_info;
    comp_info.layout                        = m_PipelineLayout;

    shader_info.codeSize = uint32_t(spirv.size_bytes()); // All shaders are in the same spirv
    shader_info.pCode    = spirv.data();

    // Tonemap Pipelines
    comp_info.stage.pName = "Tonemap";
    vkCreateComputePipelines(m_Device, nullptr, 1, &comp_info, nullptr, &m_TonemapPipeline);

    // Auto-Exposure Pipelines
    comp_info.stage.pName = "Histogram";
    vkCreateComputePipelines(m_Device, nullptr, 1, &comp_info, nullptr, &m_HistogramPipeline);

    comp_info.stage.pName = "AutoExposure";
    vkCreateComputePipelines(m_Device, nullptr, 1, &comp_info, nullptr, &m_ExposurePipeline);

    return VK_SUCCESS;
}

void vk_test::Tonemapper::deinit() {
    if (m_Device == nullptr) {
        return;
    }

    m_Alloc->destroyBuffer(m_ExposureBuffer);
    m_Alloc->destroyBuffer(m_HistogramBuffer);

    vkDestroyPipeline(m_Device, m_TonemapPipeline, nullptr);
    vkDestroyPipeline(m_Device, m_HistogramPipeline, nullptr);
    vkDestroyPipeline(m_Device, m_ExposurePipeline, nullptr);
    vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
    m_DescriptorPack.deinit();

    m_PipelineLayout  = VK_NULL_HANDLE;
    m_TonemapPipeline = VK_NULL_HANDLE;
    m_Device          = VK_NULL_HANDLE;
}

//----------------------------------
// Run the tonemapper compute shader
//
void vk_test::Tonemapper::runCompute(VkCommandBuffer                 cmd,
                                     const VkExtent2D&               size,
                                     const shaderio::TonemapperData& tonemapper,
                                     const VkDescriptorImageInfo&    in_image,
                                     const VkDescriptorImageInfo&    out_image) {
    // Push constant
    shaderio::TonemapperData tonemapper_data = tonemapper;
    tonemapper_data.autoExposureSpeed *= float(m_Timer.getSeconds());
    tonemapper_data.inputMatrix =
        shaderio::getColorCorrectionMatrix(tonemapper_data.exposure, tonemapper.temperature, tonemapper.tint);
    vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(shaderio::TonemapperData), &tonemapper_data);
    m_Timer.reset();

    // Push information to the descriptor set
    vk_test::WriteSetContainer write_set_container;
    write_set_container.append(m_DescriptorPack.makeWrite(shaderio::TonemapBinding::eImageInput), in_image);
    write_set_container.append(m_DescriptorPack.makeWrite(shaderio::TonemapBinding::eImageOutput), out_image);
    write_set_container.append(m_DescriptorPack.makeWrite(shaderio::TonemapBinding::eHistogramInputOutput), m_HistogramBuffer);
    write_set_container.append(m_DescriptorPack.makeWrite(shaderio::TonemapBinding::eLuminanceInputOutput), m_ExposureBuffer);
    vkCmdPushDescriptorSetKHR(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, write_set_container.size(), write_set_container.data());

    // Run auto-exposure histogram/exposure if enabled
    if ((tonemapper.isActive != 0) && (tonemapper.autoExposure != 0)) {
        static bool first_run = true;
        if (first_run) {
            clearHistogram(cmd);
            first_run = false;
        }

        runAutoExposureHistogram(cmd, size, in_image);
        runAutoExposure(cmd);
    }

    // Run tonemapper compute shader
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_TonemapPipeline);
    VkExtent2D group_size = vk_test::getGroupCounts(size, VkExtent2D{ TONEMAP_WORKGROUP_SIZE, TONEMAP_WORKGROUP_SIZE });
    vkCmdDispatch(cmd, group_size.width, group_size.height, 1);
}

void vk_test::Tonemapper::runAutoExposureHistogram(VkCommandBuffer cmd, const VkExtent2D& size, const VkDescriptorImageInfo& in_image) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_HistogramPipeline);
    VkExtent2D group_size = vk_test::getGroupCounts(size, TONEMAP_WORKGROUP_SIZE);
    vkCmdDispatch(cmd, group_size.width, group_size.height, 1);
    vk_test::cmdBufferMemoryBarrier(cmd,
                                    { .buffer        = m_HistogramBuffer.buffer,
                                      .srcStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                      .dstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                      .srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                      .dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT });
}

void vk_test::Tonemapper::runAutoExposure(VkCommandBuffer cmd) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_ExposurePipeline);
    vkCmdDispatch(cmd, 1, 1, 1);
    vk_test::cmdBufferMemoryBarrier(cmd,
                                    { .buffer        = m_ExposureBuffer.buffer,
                                      .srcStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                      .dstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                      .srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                      .dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT });
}

void vk_test::Tonemapper::clearHistogram(VkCommandBuffer cmd) {
    std::array<uint32_t, EXPOSURE_HISTOGRAM_SIZE> histogram_data{ 0 };
    vkCmdUpdateBuffer(cmd, m_HistogramBuffer.buffer, 0, sizeof(uint32_t) * EXPOSURE_HISTOGRAM_SIZE, histogram_data.data());

    // Add barrier to ensure update buffer completes before compute shader writes to the buffer
    vk_test::cmdBufferMemoryBarrier(cmd,
                                    { .buffer        = m_HistogramBuffer.buffer,
                                      .srcStageMask  = VK_PIPELINE_STAGE_2_CLEAR_BIT,
                                      .dstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                      .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                                      .dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT });
}
