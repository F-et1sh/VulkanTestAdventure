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
#include <hash_operations.hpp>

namespace vk_test {

    //-----------------------------------------------------------------
    // Samplers are limited in Vulkan.
    // This class is used to create and store samplers, and to avoid creating the same sampler multiple times.
    //
    // Usage:
    //      see usage_SamplerPool in sampler_pool.cpp
    //-----------------------------------------------------------------
    class SamplerPool {
    public:
        SamplerPool() = default;
        ~SamplerPool();

        // Delete copy constructor and copy assignment operator
        SamplerPool(const SamplerPool&)            = delete;
        SamplerPool& operator=(const SamplerPool&) = delete;

        // Allow move constructor and move assignment operator
        SamplerPool(SamplerPool&& other) noexcept;
        SamplerPool& operator=(SamplerPool&& other) noexcept;

        // Initialize the sampler pool with the device reference, then we can later acquire samplers
        void init(VkDevice device);
        // Destroy internal resources and reset its initial state
        void deinit();
        // Get or create VkSampler based on VkSamplerCreateInfo
        // The pNext chain may contain VkSamplerReductionModeCreateInfo as well as VkSamplerYcbcrConversionCreateInfo, but no other
        // structs are supported.
        VkResult acquireSampler(VkSampler&                 sampler,
                                const VkSamplerCreateInfo& create_info = { .sType     = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                                                                           .magFilter = VK_FILTER_LINEAR,
                                                                           .minFilter = VK_FILTER_LINEAR });

        void releaseSampler(VkSampler sampler);

    private:
        VkDevice m_Device{};

        struct SamplerState {
            VkSamplerCreateInfo                create_info{};
            VkSamplerReductionModeCreateInfo   reduction{};
            VkSamplerYcbcrConversionCreateInfo ycbr{};

            bool operator==(const SamplerState& other) const {
                return other.create_info.flags == create_info.flags &&
                       other.create_info.magFilter == create_info.magFilter &&
                       other.create_info.minFilter == create_info.minFilter &&
                       other.create_info.mipmapMode == create_info.mipmapMode &&
                       other.create_info.addressModeU == create_info.addressModeU &&
                       other.create_info.addressModeV == create_info.addressModeV &&
                       other.create_info.addressModeW == create_info.addressModeW &&
                       other.create_info.mipLodBias == create_info.mipLodBias &&
                       other.create_info.anisotropyEnable == create_info.anisotropyEnable &&
                       other.create_info.maxAnisotropy == create_info.maxAnisotropy &&
                       other.create_info.compareEnable == create_info.compareEnable &&
                       other.create_info.compareOp == create_info.compareOp &&
                       other.create_info.minLod == create_info.minLod &&
                       other.create_info.maxLod == create_info.maxLod &&
                       other.create_info.borderColor == create_info.borderColor &&
                       other.create_info.unnormalizedCoordinates == create_info.unnormalizedCoordinates &&
                       other.reduction.reductionMode == reduction.reductionMode &&
                       other.ycbr.format == ycbr.format &&
                       other.ycbr.ycbcrModel == ycbr.ycbcrModel &&
                       other.ycbr.ycbcrRange == ycbr.ycbcrRange &&
                       other.ycbr.components.r == ycbr.components.r &&
                       other.ycbr.components.g == ycbr.components.g &&
                       other.ycbr.components.b == ycbr.components.b &&
                       other.ycbr.components.a == ycbr.components.a &&
                       other.ycbr.xChromaOffset == ycbr.xChromaOffset &&
                       other.ycbr.yChromaOffset == ycbr.yChromaOffset &&
                       other.ycbr.chromaFilter == ycbr.chromaFilter &&
                       other.ycbr.forceExplicitReconstruction == ycbr.forceExplicitReconstruction;
            }
        };

        struct SamplerStateHashFn {
            std::size_t operator()(const SamplerState& s) const {
                return vk_test::hashVal(
                    s.create_info.flags,
                    s.create_info.magFilter,
                    s.create_info.minFilter,
                    s.create_info.mipmapMode,
                    s.create_info.addressModeU,
                    s.create_info.addressModeV,
                    s.create_info.addressModeW,
                    s.create_info.mipLodBias,
                    s.create_info.anisotropyEnable,
                    s.create_info.maxAnisotropy,
                    s.create_info.compareEnable,
                    s.create_info.compareOp,
                    s.create_info.minLod,
                    s.create_info.maxLod,
                    s.create_info.borderColor,
                    s.create_info.unnormalizedCoordinates,
                    s.reduction.reductionMode,
                    s.ycbr.format,
                    s.ycbr.ycbcrModel,
                    s.ycbr.ycbcrRange,
                    s.ycbr.components.r,
                    s.ycbr.components.g,
                    s.ycbr.components.b,
                    s.ycbr.components.a,
                    s.ycbr.xChromaOffset,
                    s.ycbr.yChromaOffset,
                    s.ycbr.chromaFilter,
                    s.ycbr.forceExplicitReconstruction);
            }
        };

        struct SamplerEntry {
            VkSampler sampler;
            uint32_t  reference_count;
        };

        // Stores unique samplers with their corresponding VkSamplerCreateInfo and reference counts
        std::unordered_map<SamplerState, SamplerEntry, SamplerStateHashFn> m_SamplerMap;

        // Reverse lookup map for O(1) sampler release - must stay in sync with m_samplerMap
        std::unordered_map<VkSampler, SamplerState> m_SamplerToState;

        // Mutex for thread-safe access to both maps
        mutable std::mutex m_Mutex;
    };

} // namespace vk_test
