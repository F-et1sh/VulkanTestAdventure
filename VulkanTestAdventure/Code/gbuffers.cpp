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

/*
* Modified by Farrakh
* 2025
*/

#include "pch.h"
#include "gbuffers.hpp"

#include "barriers.hpp"

vk_test::GBuffer::GBuffer(vk_test::GBuffer&& other) noexcept {
    assert(m_Info.allocator == nullptr && "Missing deinit()");
    std::swap(m_Resources, other.m_Resources);
    std::swap(m_Size, other.m_Size);
    std::swap(m_Info, other.m_Info);
    std::swap(m_DescriptorLayout, other.m_DescriptorLayout);
}

vk_test::GBuffer& vk_test::GBuffer::operator=(vk_test::GBuffer&& other) noexcept {
    if (this != &other) {
        assert(m_Info.allocator == nullptr && "Missing deinit()");
        std::swap(m_Resources, other.m_Resources);
        std::swap(m_Size, other.m_Size);
        std::swap(m_Info, other.m_Info);
        std::swap(m_DescriptorLayout, other.m_DescriptorLayout);
    }
    return *this;
}

vk_test::GBuffer::~GBuffer() {
    assert(m_Info.allocator == nullptr && "Missing deinit()");
}

void vk_test::GBuffer::init(const GBufferInitInfo& create_info) {
    assert(m_Info.color_formats.empty() && "Missing deinit()"); // The buffer must be cleared before creating a new one
    m_Info = create_info;                                       // Copy the creation info
}

void vk_test::GBuffer::deinit() {
    deinitResources();
    m_Resources        = {};
    m_Size             = {};
    m_DescriptorLayout = {};

    m_Info = {};
}

VkResult vk_test::GBuffer::update(VkCommandBuffer cmd, VkExtent2D new_size) {
    if (new_size.width == m_Size.width && new_size.height == m_Size.height) {
        return VK_SUCCESS; // Nothing to do
    }

    deinitResources();
    m_Size = new_size;
    return initResources(cmd);
}

//VkDescriptorSet vk_test::GBuffer::getDescriptorSet(uint32_t i) const {
//    return m_Resources.ui_descriptor_sets[i];
//}

VkExtent2D vk_test::GBuffer::getSize() const {
    return m_Size;
}

VkImage vk_test::GBuffer::getColorImage(uint32_t i /*= 0*/) const {
    return m_Resources.g_buffer_color[i].image;
}

VkImage vk_test::GBuffer::getDepthImage() const {
    return m_Resources.g_buffer_depth.image;
}

VkImageView vk_test::GBuffer::getColorImageView(uint32_t i /*= 0*/) const {
    return m_Resources.g_buffer_color[i].descriptor.imageView;
}

const VkDescriptorImageInfo& vk_test::GBuffer::getDescriptorImageInfo(uint32_t i /*= 0*/) const {
    return m_Resources.g_buffer_color[i].descriptor;
}

VkImageView vk_test::GBuffer::getDepthImageView() const {
    return m_Resources.g_buffer_depth.descriptor.imageView;
}

VkFormat vk_test::GBuffer::getColorFormat(uint32_t i /*= 0*/) const {
    return m_Info.color_formats[i];
}

VkFormat vk_test::GBuffer::getDepthFormat() const {
    return m_Info.depth_format;
}

VkSampleCountFlagBits vk_test::GBuffer::getSampleCount() const {
    return m_Info.sample_count;
}

float vk_test::GBuffer::getAspectRatio() const {
    if (m_Size.height == 0) {
        return 1.0F;
    }
    return float(m_Size.width) / float(m_Size.height);
}

VkResult vk_test::GBuffer::initResources(VkCommandBuffer cmd) {
    //vk_test::DebugUtil&    dutil = vk_test::DebugUtil::getInstance();
    const VkImageLayout layout{ VK_IMAGE_LAYOUT_GENERAL };
    VkDevice            device = m_Info.allocator->getDevice();

    const auto num_color = static_cast<uint32_t>(m_Info.color_formats.size());

    m_Resources.g_buffer_color.resize(num_color);
    //m_Resources.ui_image_views.resize(num_color);

    for (uint32_t c = 0; c < num_color; c++) {
        // Color image and view
        const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        const VkImageCreateInfo info  = {
             .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
             .imageType   = VK_IMAGE_TYPE_2D,
             .format      = m_Info.color_formats[c],
             .extent      = { m_Size.width, m_Size.height, 1 },
             .mipLevels   = 1,
             .arrayLayers = 1,
             .samples     = m_Info.sample_count,
             .usage       = usage,
        };
        VkImageViewCreateInfo view_info = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = m_Info.color_formats[c],
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
        };
        m_Info.allocator->createImage(m_Resources.g_buffer_color[c], info, view_info);
        //dutil.setObjectName(m_Resources.g_buffer_color[c].image, "G-Color" + std::to_string(c));
        //dutil.setObjectName(m_Resources.g_buffer_color[c].descriptor.imageView, "G-Color" + std::to_string(c));

        // UI Image color view
        //view_info.image        = m_Resources.g_buffer_color[c].image;
        //view_info.components.a = VK_COMPONENT_SWIZZLE_ONE; // Forcing the VIEW to have a 1 in the alpha channel
        //vkCreateImageView(device, &view_info, nullptr, &m_Resources.ui_image_views[c]);
        //dutil.setObjectName(m_Resources.ui_image_views[c], "UI G-Color" + std::to_string(c));

        // Set the sampler for the color attachment
        m_Resources.g_buffer_color[c].descriptor.sampler = m_Info.image_sampler;
    }

    if (m_Info.depth_format != VK_FORMAT_UNDEFINED) {
        // Depth buffer
        const VkImageCreateInfo create_info = {
            .sType       = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType   = VK_IMAGE_TYPE_2D,
            .format      = m_Info.depth_format,
            .extent      = { m_Size.width, m_Size.height, 1 },
            .mipLevels   = 1,
            .arrayLayers = 1,
            .samples     = m_Info.sample_count,
            .usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        };
        const VkImageViewCreateInfo view_info = {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .viewType         = VK_IMAGE_VIEW_TYPE_2D,
            .format           = m_Info.depth_format,
            .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, .levelCount = 1, .layerCount = 1 },
        };
        m_Info.allocator->createImage(m_Resources.g_buffer_depth, create_info, view_info);
        //dutil.setObjectName(m_Resources.g_buffer_depth.image, "G-Depth");
        //dutil.setObjectName(m_Resources.g_buffer_depth.descriptor.imageView, "G-Depth");
    }

    { // Clear all images and change layout
        std::vector<VkImageMemoryBarrier2> barriers(num_color);
        for (uint32_t c = 0; c < num_color; c++) {
            // Best layout for clearing color
            barriers[c] = vk_test::makeImageMemoryBarrier({ .image     = m_Resources.g_buffer_color[c].image,
                                                            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL });
        }
        const VkDependencyInfo dep_info{ .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                                         .imageMemoryBarrierCount = num_color,
                                         .pImageMemoryBarriers    = barriers.data() };
        vkCmdPipelineBarrier2(cmd, &dep_info);

        for (uint32_t c = 0; c < num_color; c++) {
            // Clear to avoid garbage data
            const VkClearColorValue                      clear_value = { { 1.F, 0.F, 0.F, 0.F } };
            const std::array<VkImageSubresourceRange, 1> range       = {
                { { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } }
            };
            vkCmdClearColorImage(cmd, m_Resources.g_buffer_color[c].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_value, uint32_t(range.size()), range.data());

            // Setting the layout to the final one
            barriers[c] = vk_test::makeImageMemoryBarrier(
                { .image = m_Resources.g_buffer_color[c].image, .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, .newLayout = layout });
            m_Resources.g_buffer_color[c].descriptor.imageLayout = layout;
        }
        vkCmdPipelineBarrier2(cmd, &dep_info);
    }

    // Descriptor Set for ImGUI
    //if (m_Info.descriptor_pool != nullptr) {
    //    m_Resources.ui_descriptor_sets.resize(num_color);

    //    // Create descriptor set layout (used by ImGui)
    //    const VkDescriptorSetLayoutBinding    binding = { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT };
    //    const VkDescriptorSetLayoutCreateInfo info    = {
    //           .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &binding
    //    };
    //    vkCreateDescriptorSetLayout(device, &info, nullptr, &m_DescriptorLayout);

    //    // Same layout for all color attachments
    //    std::vector<VkDescriptorSetLayout> layouts(num_color, m_DescriptorLayout);

    //    // Allocate descriptor sets
    //    std::vector<VkDescriptorImageInfo> desc_images(num_color);
    //    std::vector<VkWriteDescriptorSet>  write_desc(num_color);
    //    const VkDescriptorSetAllocateInfo  alloc_infos = {
    //         .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    //         .descriptorPool     = m_Info.descriptor_pool,
    //         .descriptorSetCount = num_color,
    //         .pSetLayouts        = layouts.data(),
    //    };
    //    vkAllocateDescriptorSets(device, &alloc_infos, m_Resources.ui_descriptor_sets.data());

    //    // Update the descriptor sets
    //    for (uint32_t d = 0; d < num_color; ++d) {
    //        desc_images[d] = { m_Info.image_sampler, m_Resources.ui_image_views[d], layout };
    //        write_desc[d]  = {
    //             .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    //             .dstSet          = m_Resources.ui_descriptor_sets[d],
    //             .descriptorCount = 1,
    //             .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //             .pImageInfo      = &desc_images[d],
    //        };
    //    }
    //    vkUpdateDescriptorSets(device, uint32_t(m_Resources.ui_descriptor_sets.size()), write_desc.data(), 0, nullptr);
    //}

    return VK_SUCCESS;
}

void vk_test::GBuffer::deinitResources() {
    if (m_Info.allocator == nullptr) {
        return; // Nothing to do
    }

    VkDevice device = m_Info.allocator->getDevice();
    if ((m_Info.descriptor_pool != nullptr) /*&& !m_Resources.ui_descriptor_sets.empty()*/) {
        //vkFreeDescriptorSets(device, m_Info.descriptor_pool, uint32_t(m_Resources.ui_descriptor_sets.size()), m_Resources.ui_descriptor_sets.data());
        vkDestroyDescriptorSetLayout(device, m_DescriptorLayout, nullptr);
        //m_Resources.ui_descriptor_sets.clear();
        m_DescriptorLayout = VK_NULL_HANDLE;
    }

    for (vk_test::Image bc : m_Resources.g_buffer_color) {
        m_Info.allocator->destroyImage(bc);
    }

    if (m_Resources.g_buffer_depth.image != VK_NULL_HANDLE) {
        m_Info.allocator->destroyImage(m_Resources.g_buffer_depth);
    }

    /*for (const VkImageView& view : m_Resources.ui_image_views) {
        vkDestroyImageView(device, view, nullptr);
    }*/
}

//--------------------------------------------------------------------------------------------------
// Usage example
//--------------------------------------------------------------------------------------------------
static void usage_GBuffer() {
    vk_test::GBuffer gbuffer;

    vk_test::ResourceAllocator allocator;
    VkSampler                  linear_sampler  = nullptr; // EX: create a linear sampler
    VkDescriptorPool           descriptor_pool = nullptr; // EX: create a descriptor pool or use the one from the app (m_app->getTextureDescriptorPool())

    // Create a G-buffer with two color images and one depth image.
    gbuffer.init({ .allocator       = &allocator,
                   .color_formats   = { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT },
                   .depth_format    = VK_FORMAT_D32_SFLOAT, // EX: use VK_FORMAT_UNDEFINED if no depth buffer is needed
                   .image_sampler   = linear_sampler,
                   .descriptor_pool = descriptor_pool });

    // Setting the size
    VkCommandBuffer cmd = nullptr; // EX: create a command buffer
    gbuffer.update(cmd, VkExtent2D{ 600, 480 });

    // Get the image views
    VkImageView color_image_view_rgba8    = gbuffer.getColorImageView(0);
    VkImageView color_image_view_rgba_f32 = gbuffer.getColorImageView(1);
    VkImageView depth_image_view          = gbuffer.getDepthImageView();

    // Display a G-Buffer using Dear ImGui like this (include <imgui.h>):
    // ImGui::Image((ImTextureID)gbuffer.getDescriptorSet(0), ImGui::GetContentRegionAvail());
}
