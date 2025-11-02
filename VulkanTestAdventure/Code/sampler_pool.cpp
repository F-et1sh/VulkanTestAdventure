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
#include "sampler_pool.hpp"

vk_test::SamplerPool::SamplerPool(SamplerPool&& other) noexcept
    : m_device(other.m_device), m_SamplerMap(std::move(other.m_SamplerMap)), m_SamplerToState(std::move(other.m_SamplerToState)) {
    // Reset the moved-from object to a valid state
    other.m_device = VK_NULL_HANDLE;
}

vk_test::SamplerPool& vk_test::SamplerPool::operator=(SamplerPool&& other) noexcept {
    if (this != &other) {
        m_device         = std::move(other.m_device);
        m_SamplerMap     = std::move(other.m_SamplerMap);
        m_SamplerToState = std::move(other.m_SamplerToState);
    }
    return *this;
}

vk_test::SamplerPool::~SamplerPool() {
    assert(m_device == VK_NULL_HANDLE && "Missing deinit()");
}

void vk_test::SamplerPool::init(VkDevice device) {
    m_device = device;
}

void vk_test::SamplerPool::deinit() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    for (const auto& entry : m_SamplerMap) {
        vkDestroySampler(m_device, entry.second.sampler, nullptr);
    }
    m_SamplerMap.clear();
    m_SamplerToState.clear();
    m_device = VK_NULL_HANDLE;
}

VkResult vk_test::SamplerPool::acquireSampler(VkSampler& sampler, const VkSamplerCreateInfo& create_info) {
    SamplerState samplerState;
    samplerState.create_info = create_info;

    // add supported extensions
    const VkBaseInStructure* ext = (const VkBaseInStructure*) create_info.pNext;
    while (ext) {
        switch (ext->sType) {
            case VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO:
                samplerState.reduction = *(const VkSamplerReductionModeCreateInfo*) ext;
                break;
            case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO:
                samplerState.ycbr = *(const VkSamplerYcbcrConversionCreateInfo*) ext;
                break;
            default:
                assert(0 && "unsupported sampler extension");
        }
        ext = ext->pNext;
    }
    // always remove pointers for comparison lookup
    samplerState.create_info.pNext = nullptr;
    samplerState.reduction.pNext   = nullptr;
    samplerState.ycbr.pNext        = nullptr;

    assert(m_device && "Initialization was missing");

    std::lock_guard<std::mutex> lock(m_Mutex);
    if (auto it = m_SamplerMap.find(samplerState); it != m_SamplerMap.end()) {
        // If found, increment reference count and return existing sampler
        it->second.reference_count++;
        sampler = it->second.sampler;
        return VK_SUCCESS;
    }

    // Otherwise, create a new sampler
    vkCreateSampler(m_device, &create_info, nullptr, &sampler);
    m_SamplerMap[samplerState] = { sampler, 1 };
    m_SamplerToState[sampler]  = samplerState;
    return VK_SUCCESS;
}

void vk_test::SamplerPool::releaseSampler(VkSampler sampler) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (sampler == VK_NULL_HANDLE)
        return;

    auto stateIt = m_SamplerToState.find(sampler);
    if (stateIt == m_SamplerToState.end()) {
        // Sampler not found - this shouldn't happen in correct usage
        assert(false && "Attempting to release unknown sampler");
        return;
    }

    auto samplerIt = m_SamplerMap.find(stateIt->second);
    assert(samplerIt != m_SamplerMap.end() && "Inconsistent sampler pool state");

    samplerIt->second.reference_count--;
    if (samplerIt->second.reference_count == 0) {
        vkDestroySampler(m_device, sampler, nullptr);
        m_SamplerMap.erase(samplerIt);
        m_SamplerToState.erase(stateIt);
    }
}

//--------------------------------------------------------------------------------------------------
// Usage example
//--------------------------------------------------------------------------------------------------
static void usage_SamplerPool() {
    VkDevice             device = nullptr; // EX: get the device from the app (m_app->getDevice())
    vk_test::SamplerPool samplerPool;
    samplerPool.init(device);

    VkSamplerCreateInfo create_info = {}; // EX: create a sampler create info or use the default one (DEFAULT_VkSamplerCreateInfo)
    VkSampler           sampler;
    samplerPool.acquireSampler(sampler, create_info);

    // Use the sampler
    // ...

    samplerPool.releaseSampler(sampler);
}
