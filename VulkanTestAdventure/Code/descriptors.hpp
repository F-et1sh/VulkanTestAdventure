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

#pragma once

#include "Resources.hpp"

namespace vk_test {

    // Descriptor Bindings
    // Helps you build descriptor set layouts by storing information about each
    // binding's type, number of descriptors, stages, and other properties.
    //
    // Usage:
    //   see usage_DescriptorBindings in descriptors.cpp
    class DescriptorBindings {
    public:
        ~DescriptorBindings() = default;

        // Adds a binding at the given `binding` index for `descriptorCount`
        // descriptors of type `descriptorType`. The resources pointed to may be
        // accessed via the given stages.
        //
        // `pImmutableSamplers` can be set to an array of `descriptorCount` samplers
        // to permanently bind them to the set layout; see
        // `VkDescriptorSetLayoutBinding::pImmutableSamplers` for more info.
        //
        // `bindingFlags` will be passed to `VkDescriptorSetLayoutBindingFlagsCreateInfo`;
        // it can be used, for instance, to let a descriptor be updated after it's bound.
        void addBinding(uint32_t                 binding,
                        VkDescriptorType         descriptor_type,
                        uint32_t                 descriptor_count,
                        VkShaderStageFlags       stage_flags,
                        const VkSampler*         p_immutable_samplers = nullptr,
                        VkDescriptorBindingFlags binding_flags        = 0);

        void addBinding(const VkDescriptorSetLayoutBinding& layout_binding, VkDescriptorBindingFlags binding_flags = 0);

        // Fills a `VkWriteDescriptorSet` struct for `descriptorCounts` descriptors,
        // starting at element `dstArrayElement`.
        //
        // If `dstArrayElement == ~0`, then the `descriptorCount` will be set to the
        // original binding's count and `dstArrayElement` to 0 -- i.e. it'll span the
        // entire binding.
        //
        // Note: the returned object is not ready to be used, as it doesn't contain
        // the information of the actual resources. You'll want to fill the image,
        // buffer, or texel buffer view info, or pass this to
        // `WriteSetContainer::append()`.
        //
        // If no entry exists for the given `binding`, returns a `VkWriteDescriptorSet`
        // with .descriptorType set to `VK_DESCRIPTOR_TYPE_MAX_ENUM`.
        VkWriteDescriptorSet getWriteSet(uint32_t        binding,
                                         VkDescriptorSet dst_set           = nullptr,
                                         uint32_t        dst_array_element = ~0,
                                         uint32_t        descriptor_count  = 1) const;

        void clear() noexcept {
            m_Bindings.clear();
            m_BindingFlags.clear();
        }

        // Once the bindings have been added, this generates the descriptor layout corresponding to the
        // bound resources.
        VkResult createDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutCreateFlags flags, VkDescriptorSetLayout* p_descriptor_set_layout) const;

        // Appends the required pool sizes for `numSets` many sets.
        // If `totalVariableCount` is non zero, it will be used to override the total requirement for the variable binding.
        // Otherwise the regular `binding.descriptorCount * numSets` is used.
        void appendPoolSizes(std::vector<VkDescriptorPoolSize>& pool_sizes, uint32_t num_sets = 1, uint32_t total_variable_count = 0) const;

        // Returns the required pool sizes for `numSets` many sets.
        // If `totalVariableCount` is non zero, it will be used to override the total requirement for the variable binding.
        // Otherwise the regular `binding.descriptorCount * numSets` is used.
        std::vector<VkDescriptorPoolSize> calculatePoolSizes(uint32_t num_sets = 1, uint32_t total_variable_count = 0) const;

        // Returns the bindings that were added
        const std::vector<VkDescriptorSetLayoutBinding>& getBindings() const { return m_Bindings; }

    private:
        std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
        std::vector<VkDescriptorBindingFlags>     m_BindingFlags;
        // Map from `VkDescriptorSetLayoutBinding::binding` to an index in the above arrays.
        // Vulkan recommends using as compact a maximum binding number as possible, so a linear array should be OK.
        std::vector<uint32_t> m_BindingToIndex;
    };

    //////////////////////////////////////////////////////////////////////////

    // Helper container for the most common descriptor set use case -- bindings
    // used to create a single layout and `numSets` descriptor sets allocated using
    // that layout.
    // It manages its own pool storage; all descriptor sets can be freed at once
    // by resetting the pool.
    //
    // Usage:
    //   see usage_DescriptorBindings() in descriptors.cpp
    class DescriptorPack {
    public:
        DescriptorPack() = default;
        DescriptorPack(DescriptorPack&& other) noexcept;
        DescriptorPack& operator=(DescriptorPack&& other) noexcept;

        ~DescriptorPack();

        // Call this function to initialize `layout`, `pool`, and `sets`.
        //
        // If `numSets` is 0, this only creates the layout.
        //
        // If `totalVariableCount` is non zero, it will be used to override the total requirement for the variable binding,
        // and `descriptorVariableCounts` must be non null and the length of `numSets`.
        // Otherwise the regular `binding.descriptorCount * numSets` is used.
        VkResult init(const DescriptorBindings&        bindings,
                      VkDevice                         device,
                      uint32_t                         num_sets                   = 1,
                      VkDescriptorSetLayoutCreateFlags layout_flags               = 0,
                      VkDescriptorPoolCreateFlags      pool_flags                 = 0,
                      uint32_t                         total_variable_count       = 0,
                      const uint32_t*                  descriptor_variable_counts = nullptr);
        void     deinit();

        VkDescriptorSetLayout               getLayout() const { return m_Layout; }
        const VkDescriptorSetLayout*        getLayoutPtr() const { return &m_Layout; }
        VkDescriptorPool                    getPool() const { return m_Pool; };
        const std::vector<VkDescriptorSet>& getSets() const { return m_Sets; }
        VkDescriptorSet                     getSet(uint32_t set_index) const { return m_Sets[set_index]; }
        const VkDescriptorSet*              getSetPtr(uint32_t set_index = 0) const { return &m_Sets[set_index]; }

        // Wrapper to get a `VkWriteDescriptorSet` for a descriptor set stored in `sets` if it's not empty.
        // Empty `sets` usage is legal in the push descriptor use-case.
        // see `DescriptorBindings::getWriteSet` for more details
        VkWriteDescriptorSet makeWrite(uint32_t binding, uint32_t set_index = 0, uint32_t dst_array_element = ~0, uint32_t descriptor_count = 1) const {
            return m_Bindings.getWriteSet(binding, m_Sets.empty() ? nullptr : m_Sets[set_index], dst_array_element, descriptor_count);
        }

    private:
        DescriptorPack(const DescriptorPack&)            = delete;
        DescriptorPack& operator=(const DescriptorPack&) = delete;

        DescriptorBindings           m_Bindings;
        VkDescriptorSetLayout        m_Layout = VK_NULL_HANDLE;
        VkDescriptorPool             m_Pool   = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> m_Sets;

        VkDevice m_Device = nullptr;
    };

    //////////////////////////////////////////////////////////////////////////

    // Helper function to create a pipeline layout.
    inline VkResult createPipelineLayout(VkDevice                               device,
                                         VkPipelineLayout*                      p_pipeline_layout,
                                         std::span<const VkDescriptorSetLayout> layouts              = {},
                                         std::span<const VkPushConstantRange>   push_constant_ranges = {}) {
        const VkPipelineLayoutCreateInfo info{ .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                               .setLayoutCount         = static_cast<uint32_t>(layouts.size()),
                                               .pSetLayouts            = layouts.data(),
                                               .pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges.size()),
                                               .pPushConstantRanges    = push_constant_ranges.data() };
        return vkCreatePipelineLayout(device, &info, nullptr, p_pipeline_layout);
    }

    // Overload so you can write {layout}, {pushConstantRange}
    inline VkResult createPipelineLayout(VkDevice                                           device,
                                         VkPipelineLayout*                                  p_pipeline_layout,
                                         std::initializer_list<const VkDescriptorSetLayout> layouts              = {},
                                         std::initializer_list<const VkPushConstantRange>   push_constant_ranges = {}) {
        return createPipelineLayout(device, p_pipeline_layout, std::span<const VkDescriptorSetLayout>(layouts.begin(), layouts.size()), std::span<const VkPushConstantRange>(push_constant_ranges.begin(), push_constant_ranges.size()));
    }

    //////////////////////////////////////////////////////////////////////////

    // Storage class for write set containers with their payload
    // Can be used to drive `vkUpdateDescriptorSets` as well
    // as `vkCmdPushDescriptorSet`
    class WriteSetContainer {
    public:
        // single element (writeSet.descriptorCount must be 1)
        void append(const VkWriteDescriptorSet& write_set, const vk_test::Buffer& buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);
        void append(const VkWriteDescriptorSet& write_set, const vk_test::AccelerationStructure& accel);
        void append(const VkWriteDescriptorSet& write_set, const vk_test::Image& image);

        void append(const VkWriteDescriptorSet& write_set, VkBuffer buffer, VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);
        void append(const VkWriteDescriptorSet& write_set, VkBufferView buffer_view);
        void append(const VkWriteDescriptorSet& write_set, const VkDescriptorBufferInfo& buffer_info);
        void append(const VkWriteDescriptorSet& write_set, VkImageView image_view, VkImageLayout image_layout, VkSampler sampler = nullptr);
        void append(const VkWriteDescriptorSet& write_set, const VkDescriptorImageInfo& image_info);
        void append(const VkWriteDescriptorSet& write_set, VkAccelerationStructureKHR accel);

        // writeSet.descriptorCount many elements
        void append(const VkWriteDescriptorSet& write_set, const vk_test::Buffer* buffers); // offset 0 and VK_WHOLE_SIZE
        void append(const VkWriteDescriptorSet& write_set, const vk_test::AccelerationStructure* accels);
        void append(const VkWriteDescriptorSet& write_set, const vk_test::Image* images);

        void append(const VkWriteDescriptorSet& write_set, const VkDescriptorBufferInfo* buffer_infos);
        void append(const VkWriteDescriptorSet& write_set, const VkDescriptorImageInfo* image_infos);
        void append(const VkWriteDescriptorSet& write_set, const VkAccelerationStructureKHR* accels);
        void append(const VkWriteDescriptorSet& write_set, const VkBufferView* buffer_views);

        void clear();

        void reserve(uint32_t count) {
            m_WriteSets.reserve(count);
            m_BufferOrImageDatas.reserve(count);
            m_AccelOrViewDatas.reserve(count);
        }

        uint32_t size() const { return uint32_t(m_WriteSets.size()); }

        // not a const function, as it will update the internal pointers if necessary
        const VkWriteDescriptorSet* data();

    private:
        union BufferOrImageData {
            VkDescriptorBufferInfo buffer;
            VkDescriptorImageInfo  image;
        };
        static_assert(sizeof(VkDescriptorBufferInfo) == sizeof(VkDescriptorImageInfo));
        union AccelOrViewData {
            VkBufferView               texel_buffer_view;
            VkAccelerationStructureKHR accel;
        };
        static_assert(sizeof(VkBufferView) == sizeof(VkAccelerationStructureKHR));

        std::vector<VkWriteDescriptorSet>                         m_WriteSets;
        std::vector<VkWriteDescriptorSetAccelerationStructureKHR> m_WriteAccels;
        std::vector<BufferOrImageData>                            m_BufferOrImageDatas;
        std::vector<AccelOrViewData>                              m_AccelOrViewDatas;
        bool                                                      m_NeedPointerUpdate = true;
    };

    //////////////////////////////////////////////////////////////////////////

} // namespace vk_test
