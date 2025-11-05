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
* SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION
* SPDX-License-Identifier: Apache-2.0
*/

/*
* Modified by Farrakh
* 2025
*/

#include "pch.h"
#include "descriptors.hpp"

namespace vk_test {

    constexpr uint32_t NO_BINDING_INDEX = ~0;

    void DescriptorBindings::addBinding(uint32_t                 binding,
                                        VkDescriptorType         descriptor_type,
                                        uint32_t                 descriptor_count,
                                        VkShaderStageFlags       stage_flags,
                                        const VkSampler*         p_immutable_samplers,
                                        VkDescriptorBindingFlags binding_flags) {

        addBinding(VkDescriptorSetLayoutBinding{ .binding            = binding,
                                                 .descriptorType     = descriptor_type,
                                                 .descriptorCount    = descriptor_count,
                                                 .stageFlags         = stage_flags,
                                                 .pImmutableSamplers = p_immutable_samplers },
                   binding_flags);
    }

    void DescriptorBindings::addBinding(const VkDescriptorSetLayoutBinding& layout_binding, VkDescriptorBindingFlags binding_flags) {
        // Update m_BindingToIndex.
        if (m_BindingToIndex.size() <= layout_binding.binding) {
            m_BindingToIndex.resize(layout_binding.binding + 1, NO_BINDING_INDEX);
        }
        m_BindingToIndex[layout_binding.binding] = static_cast<uint32_t>(m_Bindings.size());

        m_Bindings.push_back(layout_binding);
        m_BindingFlags.push_back(binding_flags);
    }

    VkWriteDescriptorSet DescriptorBindings::getWriteSet(uint32_t binding, VkDescriptorSet dst_set, uint32_t dst_array_element, uint32_t descriptor_count) const {
        VkWriteDescriptorSet write_set = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write_set.descriptorType       = VK_DESCRIPTOR_TYPE_MAX_ENUM;

        if (binding >= m_BindingToIndex.size()) {
            assert(!"`binding` was out of range!");
            return write_set;
        }
        const uint32_t i = m_BindingToIndex[binding];
        if (i == NO_BINDING_INDEX) {
            assert(!"`binding` was never added!");
            return write_set;
        }
        const VkDescriptorSetLayoutBinding& b = m_Bindings[i];

        write_set.descriptorCount = dst_array_element == ~0 ? b.descriptorCount : descriptor_count;
        write_set.descriptorType  = b.descriptorType;
        write_set.dstBinding      = binding;
        write_set.dstSet          = dst_set;
        write_set.dstArrayElement = dst_array_element == ~0 ? 0 : dst_array_element;

        assert(write_set.dstArrayElement + write_set.descriptorCount <= b.descriptorCount);

        return write_set;
    }

    VkResult DescriptorBindings::createDescriptorSetLayout(VkDevice                         device,
                                                           VkDescriptorSetLayoutCreateFlags flags,
                                                           VkDescriptorSetLayout*           p_descriptor_set_layout) const {
        VkResult                                    result;
        VkDescriptorSetLayoutBindingFlagsCreateInfo bindings_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };

        bindings_info.bindingCount  = uint32_t(m_BindingFlags.size());
        bindings_info.pBindingFlags = m_BindingFlags.data();

        VkDescriptorSetLayoutCreateInfo create_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        create_info.bindingCount                    = uint32_t(m_Bindings.size());
        create_info.pBindings                       = m_Bindings.data();
        create_info.flags                           = flags;
        create_info.pNext                           = &bindings_info;

        result = vkCreateDescriptorSetLayout(device, &create_info, nullptr, p_descriptor_set_layout);

        return result;
    }

    void DescriptorBindings::appendPoolSizes(std::vector<VkDescriptorPoolSize>& pool_sizes, uint32_t num_sets, uint32_t total_variable_count) const {

        for (size_t i = 0; i < m_Bindings.size(); i++) {
            const VkDescriptorSetLayoutBinding& it            = m_Bindings[i];
            const VkDescriptorBindingFlags      binding_flags = m_BindingFlags[i];

            // Bindings can have a zero descriptor count, used for the layout, but don't reserve storage for them.
            if (it.descriptorCount == 0) {
                continue;
            }

            uint32_t count = it.descriptorCount * num_sets;
            if ((total_variable_count != 0U) && ((binding_flags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT) != 0U)) {
                count = total_variable_count;
            }

            bool found = false;
            for (VkDescriptorPoolSize& itpool : pool_sizes) {
                if (itpool.type == it.descriptorType) {
                    itpool.descriptorCount += count;
                    found = true;
                    break;
                }
            }
            if (!found) {
                VkDescriptorPoolSize pool_size{};
                pool_size.type            = it.descriptorType;
                pool_size.descriptorCount = count;
                pool_sizes.push_back(pool_size);
            }
        }
    }

    std::vector<VkDescriptorPoolSize> DescriptorBindings::calculatePoolSizes(uint32_t num_sets, uint32_t total_variable_count) const {
        std::vector<VkDescriptorPoolSize> pool_sizes;
        appendPoolSizes(pool_sizes, num_sets, total_variable_count);
        return pool_sizes;
    }

    //////////////////////////////////////////////////////////////////////////

    VkResult DescriptorPack::init(const DescriptorBindings&        bindings,
                                  VkDevice                         device,
                                  uint32_t                         num_sets,
                                  VkDescriptorSetLayoutCreateFlags layout_flags,
                                  VkDescriptorPoolCreateFlags      pool_flags,
                                  uint32_t                         total_variable_count,
                                  const uint32_t*                  descriptor_variable_counts) {
        assert(nullptr == m_Device && "initFromBindings must not be called twice in a row!");
        m_Device = device;

        m_Bindings = bindings;

        bindings.createDescriptorSetLayout(device, layout_flags, &m_Layout);

        if (num_sets > 0) {
            // Pool
            const std::vector<VkDescriptorPoolSize> pool_sizes = bindings.calculatePoolSizes(num_sets, total_variable_count);
            const VkDescriptorPoolCreateInfo        pool_create_info{ .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                                      .flags         = pool_flags,
                                                                      .maxSets       = num_sets,
                                                                      .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
                                                                      .pPoolSizes    = pool_sizes.data() };
            vkCreateDescriptorPool(device, &pool_create_info, nullptr, &m_Pool);

            // Sets
            m_Sets.resize(num_sets);
            std::vector<VkDescriptorSetLayout> alloc_info_layouts(num_sets, m_Layout);
            VkDescriptorSetAllocateInfo        alloc_info{ .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                           .descriptorPool     = m_Pool,
                                                           .descriptorSetCount = num_sets,
                                                           .pSetLayouts        = alloc_info_layouts.data() };
            // Optional variable descriptor counts
            VkDescriptorSetVariableDescriptorCountAllocateInfo var_info{
                .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
                .descriptorSetCount = num_sets,
                .pDescriptorCounts  = descriptor_variable_counts,
            };
            if (total_variable_count > 0 && (descriptor_variable_counts != nullptr)) {
                alloc_info.pNext = &var_info;
            }

            vkAllocateDescriptorSets(device, &alloc_info, m_Sets.data());
        }

        return VK_SUCCESS;
    }

    void DescriptorPack::deinit() {
        m_Bindings.clear();
        m_Sets.clear();

        if (m_Device != nullptr) // Only run if ever initialized
        {
            vkDestroyDescriptorSetLayout(m_Device, m_Layout, nullptr);
            m_Layout = VK_NULL_HANDLE;

            vkDestroyDescriptorPool(m_Device, m_Pool, nullptr);
            m_Pool = VK_NULL_HANDLE;

            m_Device = nullptr;
        }
    }

    DescriptorPack::DescriptorPack(DescriptorPack&& other) noexcept {
        this->operator=(std::move(other));
    }

    DescriptorPack& DescriptorPack::operator=(DescriptorPack&& other) noexcept {
        assert(!m_Device && "can't move into non-empty object");

        m_Sets = std::move(other.m_Sets);

        m_Pool   = other.m_Pool;
        m_Layout = other.m_Layout;
        m_Device = other.m_Device;

        other.m_Pool   = VK_NULL_HANDLE;
        other.m_Layout = VK_NULL_HANDLE;
        other.m_Device = VK_NULL_HANDLE;

        return *this;
    }

    DescriptorPack::~DescriptorPack() {
        assert(!m_Device && "deinit() missing");
    }

    //////////////////////////////////////////////////////////////////////////

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.descriptorCount == 1);

        m_WriteSets.emplace_back(write_set).pBufferInfo = (const VkDescriptorBufferInfo*) 1;

        BufferOrImageData basics{};
        basics.buffer.buffer = buffer;
        basics.buffer.offset = offset;
        basics.buffer.range  = range;
        m_BufferOrImageDatas.emplace_back(basics);

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const VkDescriptorBufferInfo& buffer_info) {
        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.descriptorCount == 1);

        m_WriteSets.emplace_back(write_set).pBufferInfo = (const VkDescriptorBufferInfo*) 1;

        BufferOrImageData basics{};
        basics.buffer = buffer_info;
        m_BufferOrImageDatas.emplace_back(basics);

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const vk_test::Buffer& buffer, VkDeviceSize offset, VkDeviceSize range) {
        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.descriptorCount == 1);

        m_WriteSets.emplace_back(write_set).pBufferInfo = (const VkDescriptorBufferInfo*) 1;

        BufferOrImageData basics{};
        basics.buffer.buffer = buffer.buffer;
        basics.buffer.offset = offset;
        basics.buffer.range  = range;
        m_BufferOrImageDatas.emplace_back(basics);

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const vk_test::Buffer* buffers) {
        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pTexelBufferView == nullptr);

        m_WriteSets.emplace_back(write_set).pBufferInfo = (const VkDescriptorBufferInfo*) 1;

        for (uint32_t i = 0; i < write_set.descriptorCount; i++) {
            BufferOrImageData basics{};
            basics.buffer.buffer = buffers[i].buffer;
            basics.buffer.offset = 0;
            basics.buffer.range  = VK_WHOLE_SIZE;
            m_BufferOrImageDatas.emplace_back(basics);
        }

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const VkDescriptorBufferInfo* buffer_infos) {
        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pTexelBufferView == nullptr);

        m_WriteSets.emplace_back(write_set).pBufferInfo = (const VkDescriptorBufferInfo*) 1;

        for (uint32_t i = 0; i < write_set.descriptorCount; i++) {
            BufferOrImageData basics{};
            basics.buffer = buffer_infos[i];
            m_BufferOrImageDatas.emplace_back(basics);
        }

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, VkBufferView buffer_view) {
        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pBufferInfo == nullptr);
        assert(write_set.descriptorCount == 1);

        m_WriteSets.emplace_back(write_set).pTexelBufferView = (const VkBufferView*) 1;

        AccelOrViewData basics{};
        basics.texel_buffer_view = buffer_view;
        m_AccelOrViewDatas.emplace_back(basics);

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const VkBufferView* buffer_views) {
        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pBufferInfo == nullptr);

        m_WriteSets.emplace_back(write_set).pTexelBufferView = (const VkBufferView*) 1;

        for (uint32_t i = 0; i < write_set.descriptorCount; i++) {
            AccelOrViewData basics{};
            basics.texel_buffer_view = buffer_views[i];
            m_AccelOrViewDatas.emplace_back(basics);
        }

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const VkDescriptorImageInfo& image_info) {
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.pBufferInfo == nullptr);
        assert(write_set.descriptorCount == 1);

        m_WriteSets.emplace_back(write_set).pImageInfo = (const VkDescriptorImageInfo*) 1;

        BufferOrImageData basics{};
        basics.image = image_info;
        m_BufferOrImageDatas.emplace_back(basics);

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const vk_test::Image& image) {
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.pBufferInfo == nullptr);
        assert(write_set.descriptorCount == 1);
        assert(image.descriptor.imageView);

        m_WriteSets.emplace_back(write_set).pImageInfo = (const VkDescriptorImageInfo*) 1;

        BufferOrImageData basics{};
        basics.image = image.descriptor;
        m_BufferOrImageDatas.emplace_back(basics);

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, VkImageView image_view, VkImageLayout image_layout, VkSampler sampler) {
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.pBufferInfo == nullptr);
        assert(write_set.descriptorCount == 1);

        m_WriteSets.emplace_back(write_set).pImageInfo = (const VkDescriptorImageInfo*) 1;

        BufferOrImageData basics{};
        basics.image.imageView   = image_view;
        basics.image.imageLayout = image_layout;
        basics.image.sampler     = sampler;
        m_BufferOrImageDatas.emplace_back(basics);

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const vk_test::Image* images) {
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.pBufferInfo == nullptr);

        m_WriteSets.emplace_back(write_set).pImageInfo = (const VkDescriptorImageInfo*) 1;

        for (uint32_t i = 0; i < write_set.descriptorCount; i++) {
            assert(images[i].descriptor.imageView);
            BufferOrImageData basics{};
            basics.image = images[i].descriptor;
            m_BufferOrImageDatas.emplace_back(basics);
        }

        m_NeedPointerUpdate = true;
    }
    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const VkDescriptorImageInfo* image_infos) {
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.pBufferInfo == nullptr);

        m_WriteSets.emplace_back(write_set).pImageInfo = (const VkDescriptorImageInfo*) 1;

        for (uint32_t i = 0; i < write_set.descriptorCount; i++) {
            BufferOrImageData basics{};
            basics.image = image_infos[i];
            m_BufferOrImageDatas.emplace_back(basics);
        }

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, VkAccelerationStructureKHR accel) {
        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.pBufferInfo == nullptr);
        assert(write_set.descriptorCount == 1);

        m_WriteSets.emplace_back(write_set);

        AccelOrViewData basics{};
        basics.accel = accel;
        m_AccelOrViewDatas.emplace_back(basics);
        m_WriteAccels.push_back(
            { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, .accelerationStructureCount = 1 });

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const vk_test::AccelerationStructure& accel) {
        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.pBufferInfo == nullptr);
        assert(write_set.descriptorCount == 1);

        m_WriteSets.emplace_back(write_set);

        AccelOrViewData basics{};
        basics.accel = accel.accel;
        m_AccelOrViewDatas.emplace_back(basics);
        m_WriteAccels.push_back(
            { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, .accelerationStructureCount = 1 });

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const vk_test::AccelerationStructure* accels) {
        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.pBufferInfo == nullptr);

        m_WriteSets.emplace_back(write_set);

        for (uint32_t i = 0; i < write_set.descriptorCount; i++) {
            AccelOrViewData basics{};
            basics.accel = accels[i].accel;
            m_AccelOrViewDatas.emplace_back(basics);
        }

        m_WriteAccels.push_back({ .sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
                                  .accelerationStructureCount = write_set.descriptorCount });

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::append(const VkWriteDescriptorSet& write_set, const VkAccelerationStructureKHR* accels) {

        assert(write_set.pImageInfo == nullptr);
        assert(write_set.pTexelBufferView == nullptr);
        assert(write_set.pBufferInfo == nullptr);

        m_WriteSets.emplace_back(write_set);

        for (uint32_t i = 0; i < write_set.descriptorCount; i++) {
            AccelOrViewData basics{};
            basics.accel = accels[i];
            m_AccelOrViewDatas.emplace_back(basics);
        }

        m_WriteAccels.push_back({ .sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
                                  .accelerationStructureCount = write_set.descriptorCount });

        m_NeedPointerUpdate = true;
    }

    void WriteSetContainer::clear() {
        m_WriteSets.clear();
        m_WriteAccels.clear();
        m_BufferOrImageDatas.clear();
        m_AccelOrViewDatas.clear();

        m_NeedPointerUpdate = true;
    }

    const VkWriteDescriptorSet* WriteSetContainer::data() {
        if (m_NeedPointerUpdate) {
            size_t accel_write_index     = 0;
            size_t buffer_or_image_index = 0;
            size_t accel_or_view_index   = 0;

            for (size_t i = 0; i < m_WriteSets.size(); i++) {
                if (m_WriteSets[i].pBufferInfo != nullptr) {
                    m_WriteSets[i].pBufferInfo = &m_BufferOrImageDatas[buffer_or_image_index].buffer;
                    buffer_or_image_index += m_WriteSets[i].descriptorCount;
                }
                else if (m_WriteSets[i].pImageInfo != nullptr) {
                    m_WriteSets[i].pImageInfo = &m_BufferOrImageDatas[buffer_or_image_index].image;
                    buffer_or_image_index += m_WriteSets[i].descriptorCount;
                }
                else if (m_WriteSets[i].pTexelBufferView != nullptr) {
                    m_WriteSets[i].pTexelBufferView = &m_AccelOrViewDatas[accel_or_view_index].texel_buffer_view;
                    accel_or_view_index += m_WriteSets[i].descriptorCount;
                }
                else if (m_WriteSets[i].descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
                    m_WriteAccels[accel_write_index].pAccelerationStructures = &m_AccelOrViewDatas[accel_or_view_index].accel;
                    accel_or_view_index += m_WriteSets[i].descriptorCount;

                    m_WriteSets[i].pNext = &m_WriteAccels[accel_write_index];
                    accel_write_index++;
                }
            }
        }

        return m_WriteSets.empty() ? nullptr : m_WriteSets.data();
    }
} // namespace vk_test

//--------------------------------------------------------------------------------------------------
// Usage example
//--------------------------------------------------------------------------------------------------
static void usage_DescriptorBindings() {
    VkDevice        device = nullptr;
    vk_test::Buffer my_buffer_a;
    vk_test::Buffer my_buffer_b;
    uint32_t        shaderio_binding = 0;
    struct PushConstants {
        float iResolution[2];
    };

    constexpr uint32_t num_sets = 2;

    // Manually create layout and pool
    {
        // Create bindings.
        vk_test::DescriptorBindings bindings;
        // Binding `SHADERIO_BINDING` is 1 uniform buffer accessible to all stages,
        // that can be updated after binding when not in use.
        bindings.addBinding(shaderio_binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr, VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);

        // To manually create a layout and a pool and to allocate NUM_SETS sets:
        {
            VkDescriptorSetLayout dlayout = VK_NULL_HANDLE;
            bindings.createDescriptorSetLayout(device, 0, &dlayout);

            std::vector<VkDescriptorPoolSize> pool_sizes = bindings.calculatePoolSizes(num_sets);
            // Or if you have multiple descriptor layouts you'd like to allocate from a
            // single pool, you can use bindings.appendPoolSizes().

            VkDescriptorPool                 dpool = VK_NULL_HANDLE;
            const VkDescriptorPoolCreateInfo dpool_info{ .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                         .maxSets       = num_sets,
                                                         .poolSizeCount = uint32_t(pool_sizes.size()),
                                                         .pPoolSizes    = pool_sizes.data() };
            vkCreateDescriptorPool(device, &dpool_info, nullptr, &dpool);

            std::vector<VkDescriptorSet>       sets(num_sets);
            std::vector<VkDescriptorSetLayout> alloc_info_layouts(num_sets, dlayout);
            const VkDescriptorSetAllocateInfo  alloc_info{ .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                                                           .descriptorPool     = dpool,
                                                           .descriptorSetCount = num_sets,
                                                           .pSetLayouts        = alloc_info_layouts.data() };
            vkAllocateDescriptorSets(device, &alloc_info, sets.data());

            // Cleanup
            vkDestroyDescriptorPool(device, dpool, nullptr);
        }
    }

    // Or have DescriptorPack simplify the above:
    vk_test::DescriptorPack dpack;
    {
        vk_test::DescriptorBindings bindings;
        bindings.addBinding(shaderio_binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr, VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
        // The second argument, NUM_SETS here, can be left 0 when the intend is to use push descriptors, it defaults to `1`
        dpack.init(bindings, device, num_sets);
    }

    // To update DescriptorSets:
    {
        // std::vectors inside for VkWriteDescriptorSets as well as the corresponding payloads.
        vk_test::WriteSetContainer write_container;
        // makeDescriptorWrite returns a VkWriteDescriptorSet without actual binding information,
        // the append function takes care of that.

        // when preparing push descriptors, `dpack.sets[0]` would be omitted / nullptr
        write_container.append(dpack.makeWrite(shaderio_binding, 0), my_buffer_a);

        // shortcut to provide `dpack.sets[1]` (also works when dpack.sets.empty() for push descriptors)
        write_container.append(dpack.makeWrite(shaderio_binding, 1), my_buffer_b);

        vkUpdateDescriptorSets(device, write_container.size(), write_container.data(), 0, nullptr);

        // the writeContainer can also be used for push descriptors, when the VkDescriptorSet provided were nullptr
        // vkCmdPushDescriptorSet(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,  writeContainer.size(), writeContainer.data());
    }

    // To create a pipeline layout with an additional push constant range:
    VkPushConstantRange push_range{ .stageFlags = VK_SHADER_STAGE_ALL, .offset = 0, .size = sizeof(PushConstants) };
    VkPipelineLayout    pipeline_layout = VK_NULL_HANDLE;
    vk_test::createPipelineLayout(device, &pipeline_layout, { dpack.getLayout() }, { push_range });

    // Cleanup
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    dpack.deinit();
}
