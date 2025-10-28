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
#include "barriers.hpp"

namespace vk_test {
    //-----------------------------------------------------------------------------
    // Non-constexpr functions
    // This separation also allows us to avoid including volk.h in the header.

    void cmdImageMemoryBarrier(VkCommandBuffer cmd, const ImageMemoryBarrierParams& params) {
        VkImageMemoryBarrier2 barrier = makeImageMemoryBarrier(params);

        const VkDependencyInfo dep_info{ .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier };

        vkCmdPipelineBarrier2(cmd, &dep_info);
    }

    void cmdImageMemoryBarrier(VkCommandBuffer cmd, Image& image, const ImageMemoryBarrierParams& params) {
        ImageMemoryBarrierParams local_params = params;
        local_params.image                    = image.image;
        local_params.oldLayout                = image.descriptor.imageLayout;

        vk_test::cmdImageMemoryBarrier(cmd, local_params);

        image.descriptor.imageLayout = params.newLayout;
    }

    void cmdBufferMemoryBarrier(VkCommandBuffer command_buffer, const BufferMemoryBarrierParams& params) {
        VkBufferMemoryBarrier2 buffer_barrier = makeBufferMemoryBarrier(params);
        const VkDependencyInfo dep_info{
            .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .bufferMemoryBarrierCount = 1,
            .pBufferMemoryBarriers    = &buffer_barrier,
        };

        vkCmdPipelineBarrier2(command_buffer, &dep_info);
    }

    void cmdMemoryBarrier(VkCommandBuffer       cmd,
                          VkPipelineStageFlags2 src_stage_mask,
                          VkPipelineStageFlags2 dst_stage_mask,
                          VkAccessFlags2        src_access_mask /* = INFER_BARRIER_PARAMS */,
                          VkAccessFlags2        dst_access_mask /* = INFER_BARRIER_PARAMS */) {
        const VkMemoryBarrier2 memory_barrier = makeMemoryBarrier(src_stage_mask, dst_stage_mask, src_access_mask, dst_access_mask);

        const VkDependencyInfo dep_info{
            .sType              = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .memoryBarrierCount = 1,
            .pMemoryBarriers    = &memory_barrier,
        };

        vkCmdPipelineBarrier2(cmd, &dep_info);
    }

    //-----------------------------------------------------------------------------
    // BarrierContainer implementation

    void vk_test::BarrierContainer::cmdPipelineBarrier(VkCommandBuffer cmd, VkDependencyFlags dependency_flags) {
        if (memoryBarriers.empty() && bufferBarriers.empty() && imageBarriers.empty()) {
            return;
        }

        const VkDependencyInfo dep_info{
            .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .dependencyFlags          = dependency_flags,
            .memoryBarrierCount       = static_cast<uint32_t>(memoryBarriers.size()),
            .pMemoryBarriers          = memoryBarriers.data(),
            .bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size()),
            .pBufferMemoryBarriers    = bufferBarriers.data(),
            .imageMemoryBarrierCount  = static_cast<uint32_t>(imageBarriers.size()),
            .pImageMemoryBarriers     = imageBarriers.data(),
        };
        vkCmdPipelineBarrier2(cmd, &dep_info);
    }

    void vk_test::BarrierContainer::appendOptionalLayoutTransition(Image& image, VkImageMemoryBarrier2 image_barrier) {
        if (image.descriptor.imageLayout == image_barrier.newLayout) {
            return;
        }

        image_barrier.image = image.image;
        imageBarriers.push_back(image_barrier);

        image.descriptor.imageLayout = image_barrier.newLayout;
    }

    void vk_test::BarrierContainer::clear() {
        memoryBarriers.clear();
        bufferBarriers.clear();
        imageBarriers.clear();
    }

} // namespace vk_test
