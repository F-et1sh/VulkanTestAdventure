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
#include "resource_allocator.hpp"
#include "timers.hpp"
#include <../Shaders/tonemap_io.h.slang>
#include <descriptors.hpp>

namespace vk_test {

    class Tonemapper {
    public:
        Tonemapper() = default;
        ~Tonemapper() { assert(m_Device == VK_NULL_HANDLE); } //  "Missing to call deinit"

        VkResult init(vk_test::ResourceAllocator* alloc, std::span<const uint32_t> spirv);
        void     deinit();

        void runCompute(VkCommandBuffer                 cmd,
                        const VkExtent2D&               size,
                        const shaderio::TonemapperData& tonemapper,
                        const VkDescriptorImageInfo&    in_image,
                        const VkDescriptorImageInfo&    out_image);

    private:
        // Add new methods for histogram-based auto-exposure
        void runAutoExposureHistogram(VkCommandBuffer cmd, const VkExtent2D& size, const VkDescriptorImageInfo& in_image);
        void runAutoExposure(VkCommandBuffer cmd);
        void clearHistogram(VkCommandBuffer cmd);

        vk_test::ResourceAllocator* m_Alloc{};

        VkDevice                m_Device{};
        vk_test::DescriptorPack m_DescriptorPack;
        VkPipelineLayout        m_PipelineLayout{};
        VkPipeline              m_TonemapPipeline{};
        VkPipeline              m_HistogramPipeline{};
        VkPipeline              m_ExposurePipeline{};

        vk_test::PerformanceTimer m_Timer; // Timer for performance measurement

        // Auto-Exposure
        vk_test::Buffer m_ExposureBuffer;
        vk_test::Buffer m_HistogramBuffer;
    };

} // namespace vk_test
