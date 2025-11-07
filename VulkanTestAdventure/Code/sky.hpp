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

#pragma once
#include <resource_allocator.hpp>
#include "../Shaders/sky_io.h.slang"

namespace vk_test {
    template <typename SkyParams>
    class SkyBase {
    public:
        SkyBase() = default;
        virtual ~SkyBase() { assert(m_Shader == VK_NULL_HANDLE); } // "Missing to call deinit"

        void init(vk_test::ResourceAllocator* alloc, std::span<const uint32_t> spirv) {
            m_Device = alloc->getDevice();

            // Binding layout
            constexpr static auto layout_bindings = std::to_array<VkDescriptorSetLayoutBinding>({
                { .binding         = shaderio::SkyBindings::eSkyOutImage,
                  .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                  .descriptorCount = 1,
                  .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT },
            });

            // Descriptor set layout
            const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info{
                .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT,
                .bindingCount = uint32_t(layout_bindings.size()),
                .pBindings    = layout_bindings.data(),
            };
            vkCreateDescriptorSetLayout(m_Device, &descriptor_set_layout_info, nullptr, &m_DescriptorSetLayout);

            // Push constant
            auto push_constant_ranges = std::to_array<VkPushConstantRange>({
                { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                  .offset     = 0,
                  .size       = sizeof(SkyParams) + sizeof(glm::mat4) },
            });

            // Pipeline layout
            const VkPipelineLayoutCreateInfo pipeline_layout_info{
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount         = 1,
                .pSetLayouts            = &m_DescriptorSetLayout,
                .pushConstantRangeCount = uint32_t(push_constant_ranges.size()),
                .pPushConstantRanges    = push_constant_ranges.data(),
            };
            vkCreatePipelineLayout(m_Device, &pipeline_layout_info, nullptr, &m_PipelineLayout);

            // Shader
            VkShaderCreateInfoEXT shader_info{
                .sType                  = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT,
                .stage                  = VK_SHADER_STAGE_COMPUTE_BIT,
                .codeType               = VK_SHADER_CODE_TYPE_SPIRV_EXT,
                .codeSize               = spirv.size_bytes(),
                .pCode                  = spirv.data(),
                .pName                  = "main",
                .setLayoutCount         = 1,
                .pSetLayouts            = &m_DescriptorSetLayout,
                .pushConstantRangeCount = uint32_t(push_constant_ranges.size()),
                .pPushConstantRanges    = push_constant_ranges.data(),
            };
            vkCreateShadersEXT(m_Device, 1U, &shader_info, nullptr, &m_Shader);
        }

        void deinit() {
            vkDestroyShaderEXT(m_Device, m_Shader, nullptr);
            vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
            vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
            m_Shader              = VK_NULL_HANDLE;
            m_DescriptorSetLayout = VK_NULL_HANDLE;
            m_PipelineLayout      = VK_NULL_HANDLE;
        }

        void runCompute(VkCommandBuffer              cmd,
                        const VkExtent2D&            size,
                        const glm::mat4&             view_matrix,
                        const glm::mat4&             proj_matrix,
                        const SkyParams&             sky_param,
                        const VkDescriptorImageInfo& io_image) {
            // Bind shader
            const VkShaderStageFlagBits stages[1] = { VK_SHADER_STAGE_COMPUTE_BIT };
            vkCmdBindShadersEXT(cmd, 1, stages, &m_Shader);

            // Remove the translation from the view matrix
            glm::mat4 view_no_trans = view_matrix;
            view_no_trans[3]        = { 0.0F, 0.0F, 0.0F, 1.0F };
            glm::mat4 mvp           = glm::inverse(proj_matrix * view_no_trans); // This will be to have a world direction vector pointing to the pixel

            // Push constant
            vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SkyParams), &sky_param);
            vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(SkyParams), sizeof(glm::mat4), &mvp);

            // Update descriptor sets
            VkWriteDescriptorSet write_descriptor_set[1]{
                {
                    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet          = 0,
                    .dstBinding      = shaderio::SkyBindings::eSkyOutImage,
                    .descriptorCount = 1,
                    .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    .pImageInfo      = &io_image,
                },
            };
            vkCmdPushDescriptorSetKHR(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PipelineLayout, 0, 1, write_descriptor_set);

            // Dispatching the compute job
            vkCmdDispatch(cmd, (size.width + 15) / 16, (size.height + 15) / 16, 1);
        }

    protected:
        VkDevice              m_Device{};
        VkDescriptorSetLayout m_DescriptorSetLayout{};
        VkPipelineLayout      m_PipelineLayout{};
        VkShaderEXT           m_Shader{};
    };

    // Define specific types
    using SkySimple   = SkyBase<shaderio::SkySimpleParameters>;
    using SkyPhysical = SkyBase<shaderio::SkyPhysicalParameters>;

} // namespace vk_test
