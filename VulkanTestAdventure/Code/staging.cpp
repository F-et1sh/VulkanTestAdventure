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

#include "pch.h"
#include "staging.hpp"
#include "barriers.hpp"

namespace vk_test {

    StagingUploader::StagingUploader(StagingUploader&& other) noexcept {
        {
            std::swap(m_Batch, other.m_Batch);
            std::swap(m_ResourceAllocator, other.m_ResourceAllocator);
            std::swap(m_StagingResourcesSize, other.m_StagingResourcesSize);
            std::swap(m_StagingResources, other.m_StagingResources);
        }
    }

    vk_test::StagingUploader& StagingUploader::operator=(StagingUploader&& other) noexcept {
        if (this != &other) {
            assert(m_ResourceAllocator == nullptr && "deinit not called prior move assignment");

            std::swap(m_Batch, other.m_Batch);
            std::swap(m_ResourceAllocator, other.m_ResourceAllocator);
            std::swap(m_StagingResourcesSize, other.m_StagingResourcesSize);
            std::swap(m_StagingResources, other.m_StagingResources);
        }
        return *this;
    }

    void StagingUploader::init(ResourceAllocator* resource_allocator, bool enable_layout_barriers) {
        assert(m_ResourceAllocator == nullptr);
        m_ResourceAllocator    = resource_allocator;
        m_EnableLayoutBarriers = enable_layout_barriers;
    }

    void StagingUploader::deinit() {
        if (m_ResourceAllocator != nullptr) {
            releaseStaging(true);
            assert(m_StagingResources.empty() && m_StagingResourcesSize == 0); // must have released all staged uploads
        }
        m_ResourceAllocator = nullptr;
    }

    void StagingUploader::setEnableLayoutBarriers(bool enable_layout_barriers) {
        m_EnableLayoutBarriers = enable_layout_barriers;
    }

    vk_test::ResourceAllocator* StagingUploader::getResourceAllocator() {
        assert(m_ResourceAllocator);
        return m_ResourceAllocator;
    }

    VkResult StagingUploader::acquireStagingSpace(BufferRange& staging_space, size_t data_size, const void* data, const SemaphoreState& semaphore_state) {
        StagingResource staging_resource;
        staging_resource.semaphore_state = semaphore_state;

        // VMA_MEMORY_USAGE_AUTO_PREFER_HOST staging memory is meant to not cost additional device memory
        //
        // VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT staging memory is filled sequentially
        // VMA_ALLOCATION_CREATE_MAPPED_BIT staging memory is filled through pointer access
        //
        // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT we want to avoid having to call "vkFlushMappedMemoryRanges"
        //
        // As of writing VMA doesn't have a simple usage that guarantees VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        // (only the deprecated VMA_MEMORY_USAGE_CPU_ONLY did)

        VmaAllocationCreateInfo alloc_info = {
            .flags         = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage         = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        };

        const VkBufferUsageFlags2CreateInfo buffer_usage_flags2_create_info{
            .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO,
            .usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT_KHR | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
        };

        const VkBufferCreateInfo buffer_info{
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext       = &buffer_usage_flags2_create_info,
            .size        = data_size,
            .usage       = 0,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        // Create a staging buffer
        m_ResourceAllocator->createBuffer(staging_resource.buffer, buffer_info, alloc_info);

        if (staging_resource.buffer.mapping == nullptr) {
            m_ResourceAllocator->destroyBuffer(staging_resource.buffer);
            return VK_ERROR_MEMORY_MAP_FAILED;
        }

        if (data != nullptr) {
            memcpy(staging_resource.buffer.mapping, data, data_size);
        }

        m_StagingResourcesSize += data_size;
        m_StagingResources.emplace_back(staging_resource);

        staging_space.buffer  = staging_resource.buffer.buffer;
        staging_space.offset  = 0;
        staging_space.range   = data_size;
        staging_space.address = staging_resource.buffer.address;
        staging_space.mapping = staging_resource.buffer.mapping;

        return VK_SUCCESS;
    }

    void StagingUploader::cancelAppended() {
        // let's use clear rather than m_Batch = {};
        // to avoid heap allocations

        m_Batch.copy_buffer_image_infos.clear();
        m_Batch.copy_buffer_image_regions.clear();
        m_Batch.copy_buffer_infos.clear();
        m_Batch.copy_buffer_regions.clear();
        m_Batch.pre.clear();
        m_Batch.post.clear();
        m_Batch.staging_size  = 0;
        m_Batch.transfer_only = false;
    }

    bool StagingUploader::isAppendedEmpty() const {
        return m_Batch.copy_buffer_image_infos.empty() && m_Batch.copy_buffer_infos.empty();
    }

    void StagingUploader::beginTransferOnly() {
        m_Batch.transfer_only = true;
    }

    VkResult StagingUploader::appendBuffer(const vk_test::Buffer& buffer,
                                           VkDeviceSize           buffer_offset,
                                           VkDeviceSize           data_size,
                                           const void*            data,
                                           const SemaphoreState&  semaphore_state) {
        // allow empty without throwing error
        if (data_size == 0) {
            return VK_SUCCESS;
        }

        if (data_size == VK_WHOLE_SIZE) {
            data_size = buffer.bufferSize;
        }

        assert(data);
        assert(buffer_offset + data_size <= buffer.bufferSize);
        assert(buffer.buffer);

        if (buffer.mapping != nullptr) {
            memcpy(buffer.mapping + buffer_offset, data, data_size);
        }
        else {
            BufferRange staging_space;
            acquireStagingSpace(staging_space, data_size, data, semaphore_state);

            VkBufferCopy2 copy_region_info{
                .sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                .srcOffset = staging_space.offset,
                .dstOffset = buffer_offset,
                .size      = data_size,
            };

            VkCopyBufferInfo2 copy_buffer_info{
                .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                .srcBuffer   = staging_space.buffer,
                .dstBuffer   = buffer.buffer,
                .regionCount = 1,
                .pRegions    = nullptr, // set when calling `cmdUploadAppended`
            };

            m_Batch.staging_size += data_size;
            m_Batch.copy_buffer_regions.emplace_back(copy_region_info);
            m_Batch.copy_buffer_infos.emplace_back(copy_buffer_info);
        }

        return VK_SUCCESS;
    }

    VkResult StagingUploader::appendBufferRange(const vk_test::BufferRange& buffer_range, const void* data, const SemaphoreState& semaphore_state) {
        // allow empty without throwing error
        if (buffer_range.range == 0) {
            return VK_SUCCESS;
        }

        assert(data);
        assert(buffer_range.buffer);

        if (buffer_range.mapping != nullptr) {
            memcpy(buffer_range.mapping, data, buffer_range.range);
        }
        else {
            BufferRange staging_space;
            acquireStagingSpace(staging_space, buffer_range.range, data, semaphore_state);

            VkBufferCopy2 copy_region_info{
                .sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                .srcOffset = staging_space.offset,
                .dstOffset = buffer_range.offset,
                .size      = buffer_range.range,
            };

            VkCopyBufferInfo2 copy_buffer_info{
                .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                .srcBuffer   = staging_space.buffer,
                .dstBuffer   = buffer_range.buffer,
                .regionCount = 1,
                .pRegions    = nullptr, // set when calling `cmdUploadAppended`
            };

            m_Batch.staging_size += buffer_range.range;
            m_Batch.copy_buffer_regions.emplace_back(copy_region_info);
            m_Batch.copy_buffer_infos.emplace_back(copy_buffer_info);
        }

        return VK_SUCCESS;
    }

    VkResult StagingUploader::appendBufferMapping(const vk_test::Buffer& buffer,
                                                  VkDeviceSize           buffer_offset,
                                                  VkDeviceSize           data_size,
                                                  void*&                 upload_mapping,
                                                  const SemaphoreState&  semaphore_state) {
        upload_mapping = nullptr;

        // allow empty without throwing error
        if (data_size == 0) {
            return VK_SUCCESS;
        }

        if (data_size == VK_WHOLE_SIZE) {
            data_size = buffer.bufferSize;
        }

        assert(buffer.buffer);
        assert(buffer_offset + data_size <= buffer.bufferSize);

        if (buffer.mapping != nullptr) {
            upload_mapping = buffer.mapping + buffer_offset;

            return VK_SUCCESS;
        }

        BufferRange staging_space;
        acquireStagingSpace(staging_space, data_size, nullptr, semaphore_state);

        upload_mapping = staging_space.mapping;

        VkBufferCopy2 copy_region_info{
            .sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
            .srcOffset = staging_space.offset,
            .dstOffset = buffer_offset,
            .size      = data_size,
        };

        VkCopyBufferInfo2 copy_buffer_info{
            .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
            .srcBuffer   = staging_space.buffer,
            .dstBuffer   = buffer.buffer,
            .regionCount = 1,
            .pRegions    = nullptr, // set when calling `cmdUploadAppended`
        };

        m_Batch.staging_size += data_size;
        m_Batch.copy_buffer_regions.emplace_back(copy_region_info);
        m_Batch.copy_buffer_infos.emplace_back(copy_buffer_info);

        return VK_SUCCESS;
    }

    VkResult StagingUploader::appendBufferRangeMapping(const vk_test::BufferRange& buffer_range, void*& upload_mapping, const SemaphoreState& semaphore_state) {
        upload_mapping = nullptr;

        // allow empty without throwing error
        if (buffer_range.range == 0) {
            return VK_SUCCESS;
        }

        assert(buffer_range.buffer);

        if (buffer_range.mapping != nullptr) {
            upload_mapping = buffer_range.mapping;

            return VK_SUCCESS;
        }

        BufferRange staging_space;
        acquireStagingSpace(staging_space, buffer_range.range, nullptr, semaphore_state);

        upload_mapping = staging_space.mapping;

        VkBufferCopy2 copy_region_info{
            .sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
            .srcOffset = staging_space.offset,
            .dstOffset = buffer_range.offset,
            .size      = buffer_range.range,
        };

        VkCopyBufferInfo2 copy_buffer_info{
            .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
            .srcBuffer   = staging_space.buffer,
            .dstBuffer   = buffer_range.buffer,
            .regionCount = 1,
            .pRegions    = nullptr, // set when calling `cmdUploadAppended`
        };

        m_Batch.staging_size += buffer_range.range;
        m_Batch.copy_buffer_regions.emplace_back(copy_region_info);
        m_Batch.copy_buffer_infos.emplace_back(copy_buffer_info);

        return VK_SUCCESS;
    }

    VkResult StagingUploader::appendImage(vk_test::Image& image, size_t data_size, const void* data, VkImageLayout new_layout, const SemaphoreState& semaphore_state) {
        BufferRange staging_space;
        acquireStagingSpace(staging_space, data_size, data, semaphore_state);

        bool layout_allows_copy = image.descriptor.imageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || image.descriptor.imageLayout == VK_IMAGE_LAYOUT_GENERAL || image.descriptor.imageLayout == VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;

        VkImageLayout dst_image_layout = image.descriptor.imageLayout;

        if (m_EnableLayoutBarriers && !layout_allows_copy) {
            VkImageMemoryBarrier2 barrier = makeImageMemoryBarrier(
                { .image = image.image, .oldLayout = image.descriptor.imageLayout, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL });
            modifyImageBarrier(barrier);

            dst_image_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            m_Batch.pre.imageBarriers.push_back(barrier);
        }

        // Copy buffer data to the image
        const VkBufferImageCopy2 copy_buffer_image_region{
            .sType             = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
            .bufferOffset      = staging_space.offset,
            .bufferRowLength   = 0, // tightly packed
            .bufferImageHeight = 0, // tightly packed
            .imageSubresource  = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
            .imageOffset       = { 0, 0, 0 },
            .imageExtent       = image.extent,
        };

        VkCopyBufferToImageInfo2 copy_buffer_to_image_info{
            .sType          = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
            .srcBuffer      = staging_space.buffer,
            .dstImage       = image.image,
            .dstImageLayout = dst_image_layout,
            .regionCount    = 1,
            .pRegions       = nullptr, // set when calling `cmdUploadAppended`
        };

        m_Batch.staging_size += data_size;
        m_Batch.copy_buffer_image_regions.emplace_back(copy_buffer_image_region);
        m_Batch.copy_buffer_image_infos.emplace_back(copy_buffer_to_image_info);

        if (m_EnableLayoutBarriers && (!layout_allows_copy || new_layout != VK_IMAGE_LAYOUT_UNDEFINED)) {
            if (new_layout != VK_IMAGE_LAYOUT_UNDEFINED) {
                image.descriptor.imageLayout = new_layout;
            }

            VkImageMemoryBarrier2 barrier =
                makeImageMemoryBarrier({ .image = image.image, .oldLayout = dst_image_layout, .newLayout = image.descriptor.imageLayout });

            modifyImageBarrier(barrier);

            m_Batch.post.imageBarriers.push_back(barrier);
        }

        return VK_SUCCESS;
    }

    VkResult StagingUploader::appendImageSub(vk_test::Image&                 image,
                                             const VkOffset3D&               offset,
                                             const VkExtent3D&               extent,
                                             const VkImageSubresourceLayers& subresource,
                                             size_t                          data_size,
                                             const void*                     data,
                                             VkImageLayout                   new_layout /*= VK_IMAGE_LAYOUT_UNDEFINED*/,
                                             const SemaphoreState&           semaphore_state /*= {}*/) {
        BufferRange staging_space;
        acquireStagingSpace(staging_space, data_size, data, semaphore_state);

        bool layout_allows_copy = image.descriptor.imageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || image.descriptor.imageLayout == VK_IMAGE_LAYOUT_GENERAL || image.descriptor.imageLayout == VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;

        VkImageLayout dst_image_layout = image.descriptor.imageLayout;

        if (m_EnableLayoutBarriers && !layout_allows_copy) {
            VkImageMemoryBarrier2 barrier = makeImageMemoryBarrier(
                { .image = image.image, .oldLayout = image.descriptor.imageLayout, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL });
            modifyImageBarrier(barrier);

            dst_image_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            m_Batch.pre.imageBarriers.push_back(barrier);
        }

        // Copy buffer data to the image
        const VkBufferImageCopy2 copy_buffer_image_region{
            .sType             = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
            .bufferOffset      = staging_space.offset,
            .bufferRowLength   = 0, // tightly packed
            .bufferImageHeight = 0, // tightly packed
            .imageSubresource  = subresource,
            .imageOffset       = offset,
            .imageExtent       = extent,
        };

        VkCopyBufferToImageInfo2 copy_buffer_to_image_info{
            .sType          = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2,
            .srcBuffer      = staging_space.buffer,
            .dstImage       = image.image,
            .dstImageLayout = dst_image_layout,
            .regionCount    = 1,
            .pRegions       = nullptr, // set when calling `cmdUploadAppended`
        };

        m_Batch.staging_size += data_size;
        m_Batch.copy_buffer_image_regions.emplace_back(copy_buffer_image_region);
        m_Batch.copy_buffer_image_infos.emplace_back(copy_buffer_to_image_info);

        if (m_EnableLayoutBarriers && (!layout_allows_copy || new_layout != VK_IMAGE_LAYOUT_UNDEFINED)) {
            if (new_layout != VK_IMAGE_LAYOUT_UNDEFINED) {
                image.descriptor.imageLayout = new_layout;
            }

            VkImageMemoryBarrier2 barrier =
                makeImageMemoryBarrier({ .image = image.image, .oldLayout = dst_image_layout, .newLayout = image.descriptor.imageLayout });

            modifyImageBarrier(barrier);

            m_Batch.post.imageBarriers.push_back(barrier);
        }

        return VK_SUCCESS;
    }

    bool StagingUploader::checkAppendedSize(size_t limit_in_bytes, size_t added_size) const {
        return (m_Batch.staging_size != 0U) && (m_Batch.staging_size + added_size) > limit_in_bytes;
    }

    void StagingUploader::cmdUploadAppended(VkCommandBuffer cmd) {
        if (m_EnableLayoutBarriers) {
            m_Batch.pre.cmdPipelineBarrier(cmd, 0);
        }

        for (size_t i = 0; i < m_Batch.copy_buffer_infos.size(); i++) {
            m_Batch.copy_buffer_infos[i].pRegions = &m_Batch.copy_buffer_regions[i];
            vkCmdCopyBuffer2(cmd, &m_Batch.copy_buffer_infos[i]);
        }

        for (size_t i = 0; i < m_Batch.copy_buffer_image_infos.size(); i++) {
            m_Batch.copy_buffer_image_infos[i].pRegions = &m_Batch.copy_buffer_image_regions[i];
            vkCmdCopyBufferToImage2(cmd, &m_Batch.copy_buffer_image_infos[i]);
        }

        if (m_EnableLayoutBarriers) {
            m_Batch.post.cmdPipelineBarrier(cmd, 0);
        }

        // reset
        cancelAppended();
    }

    void StagingUploader::modifyImageBarrier(VkImageMemoryBarrier2& barrier) const {
        if (m_Batch.transfer_only) {
            barrier.dstAccessMask &= (VK_ACCESS_2_TRANSFER_WRITE_BIT | VK_ACCESS_2_TRANSFER_READ_BIT);
            barrier.srcAccessMask &= (VK_ACCESS_2_TRANSFER_WRITE_BIT | VK_ACCESS_2_TRANSFER_READ_BIT);
            barrier.dstStageMask &= VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
            barrier.srcStageMask &= VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT;
            assert(barrier.dstAccessMask && barrier.srcStageMask);
        }
    }

    void StagingUploader::releaseStaging(bool force_all) {
        VkDevice device = m_ResourceAllocator->getDevice();

        size_t original_count = m_StagingResources.size();
        size_t read_idx       = 0;
        size_t write_idx      = 0;

        // compact as we iterate

        for (size_t read_idx = 0; read_idx < m_StagingResources.size(); read_idx++) {
            StagingResource& staging_resource = m_StagingResources[read_idx];

            // always release with forceAll,
            // also if semaphore_state is invalid,
            // otherwise test if it was signaled
            bool can_release =
                force_all || (!staging_resource.semaphore_state.isValid()) || staging_resource.semaphore_state.testSignaled(device);

            if (can_release) {
                m_StagingResourcesSize -= staging_resource.buffer.bufferSize;
                m_ResourceAllocator->destroyBuffer(staging_resource.buffer);

                staging_resource.semaphore_state = {};
            }
            else if (read_idx != write_idx) {
                m_StagingResources[write_idx++] = std::move(staging_resource);
            }
            else if (!force_all) {
                write_idx++;
            }
        }

        m_StagingResources.resize(write_idx);
    }

} // namespace vk_test

//--------------------------------------------------------------------------------------------------
// Usage example
//--------------------------------------------------------------------------------------------------
static void usage_StagingUploader() {
    VkDevice                   device{};
    VkResult                   result{};
    vk_test::ResourceAllocator resource_allocator{};

    vk_test::StagingUploader staging_uploader;
    staging_uploader.init(&resource_allocator);

    //////////////////////////////////////////////////////////////////////////
    // simple example, relying on device wait idle, not using SemaphoreState
    {
        // Create buffer
        vk_test::Buffer buffer;

        // we prefer device memory and set a few bits that allow device-mappable memory to be used, but doesn't enforce it
        resource_allocator.createBuffer(buffer, 256, VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

        // Upload data
        std::vector<float> data      = { 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F, 9.0F, 10.0F };
        size_t             data_size = data.size() * sizeof(float);
        uint32_t           offset    = 0;

        // The stagingUploader will detect if the buffer was mappable directly then copy there directly,
        // otherwise copy through a temporary staging buffer.
        //
        // Note we consume the provided pointers directly here, but underlying vulkan commands are "appended"
        // and need to be executed later.
        staging_uploader.appendBuffer(buffer, offset, data_size, data.data());

        // Execute the upload of all previously appended data to the GPU (if necessary)
        VkCommandBuffer cmd = nullptr; // EX: create a command buffer
        staging_uploader.cmdUploadAppended(cmd);

        // submit command buffer
        //vkQueueSubmit(...);

        // device wait idle ensures cmd has completed
        vkDeviceWaitIdle(device);

        // safe to release everything after
        staging_uploader.releaseStaging();
    }

    //////////////////////////////////////////////////////////////////////////
    // batched,  relying on device wait idle for release, not using SemaphoreState
    {
        // get command buffer somehow
        VkCommandBuffer cmd{};

        // in this scenario we want to upload a lot of stuff, but we want to keep
        // an upper bound to how much temporary staging memory we use (1 GiB here).

        // we want the staging uploader to manage image layout transitions
        staging_uploader.setEnableLayoutBarriers(true);

        std::vector<vk_test::Image>     my_image_textures;
        std::vector<std::span<uint8_t>> my_image_datas;
        for (size_t i = 0; i < my_image_textures.size(); i++) {
            bool is_last = i == my_image_textures.size() - 1;

            // we are not using the semaphore_state in this loop because we intend to
            // use device wait idle and submit in multiple batches anyway

            // this will handle the transition from the current `myImageTextures[i].descriptor.imageLayout` to `VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL`
            // as well as the intermediate transition to `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL`
            result = staging_uploader.appendImage(my_image_textures[i], my_image_datas[i], VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);

            // flush upload if we reached a gigabyte or if we are the last resource
            if (is_last || staging_uploader.checkAppendedSize(1024 * 1024 * 1024)) {
                // handles transfers and barriers for layout transitions
                staging_uploader.cmdUploadAppended(cmd);

                // submit cmd buffer to queue
                cmd;

                vkDeviceWaitIdle(device);

                staging_uploader.releaseStaging();

                // get a new command buffer
                cmd;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // using semaphore state to track deletion of temporary resources

    {
        // imagine we are updating this buffer every frame
        // create buffer
        std::vector<float> my_data;
        vk_test::Buffer    my_buffer;
        VkResult           result = resource_allocator.createBuffer(my_buffer, std::span(my_data).size_bytes(), VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT);

        // track its completion through a timeline semaphore
        VkSemaphore timeline_semaphore{};
        uint64_t    timeline_value = 1;

        // frame loop
        while (true) {
            // release staging resources from past frames based on their SemaphoreState
            staging_uploader.releaseStaging();

            // get command buffer somehow
            VkCommandBuffer cmd{};

            vk_test::SemaphoreState semaphore_state = vk_test::SemaphoreState::makeFixed(timeline_semaphore, timeline_value);

            // the staging uploader provides two ways to fill a buffer:
            if (true) {
                // either providing the data in full
                // and we copy in full via memcpy
                result = staging_uploader.appendBuffer(my_buffer, 0, std::span(my_data), semaphore_state);
            }
            else {
                // or get a pointer
                float* mapping_pointer = nullptr;
                result                 = staging_uploader.appendBufferMapping(my_buffer, 0, std::span(my_data).size_bytes(), mapping_pointer, semaphore_state);
                // manually fill the pointer with sequential writes
            }

            // record all potential copy and barrier operations
            staging_uploader.cmdUploadAppended(cmd);
            // submit cmd buffer to queue signaling the timelineValue
            cmd;

            // next frame uses new timelineValue
            timeline_value++;
        }
    }
}
