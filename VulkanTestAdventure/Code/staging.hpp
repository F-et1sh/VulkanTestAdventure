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

/*
* Modified by Farrakh
* 2025
*/

#pragma once

#include "semaphore.hpp"
#include "barriers.hpp"
#include "resource_allocator.hpp"

//-----------------------------------------------------------------
// StagingUploader is a class that allows to upload data to the GPU.
//
// Usage:
//      see usage_StagingUploader in staging.cpp
//-----------------------------------------------------------------

namespace vk_test {

    class StagingUploader {
    public:
        StagingUploader()                                  = default;
        StagingUploader(const StagingUploader&)            = delete;
        StagingUploader& operator=(const StagingUploader&) = delete;
        StagingUploader(StagingUploader&& other) noexcept;
        StagingUploader& operator=(StagingUploader&& other) noexcept;
        virtual ~StagingUploader() {
            assert(isAppendedEmpty() && "Did you forget cmdUploadAppended() or cancelAppended()");
            assert(m_ResourceAllocator == nullptr && "Missing deinit()");
        }

        // explicit lifetime of resourceAllocator must be ensured externally
        void init(ResourceAllocator* resource_allocator, bool enable_layout_barriers = false);

        // deinit implicitly calls `releaseStaging(true)`
        void deinit();

        void setEnableLayoutBarriers(bool enable_layout_barriers);

        ResourceAllocator* getResourceAllocator();

        // All temporary staging resources are associated with the provided SemaphoreState.

        // releases temporary staging resources based on SemaphoreState
        // if `!SemaphoreState.isValid ` then immediately released
        // otherwise runs `SemaphoreState.testSignaled`
        // if `forceAll` is true, then we assume it's safe delete all resources
        // this typically requires a device wait idle in advance.
        // virtual so derived classes can implement staging space management by different means.
        virtual void releaseStaging(bool force_all = false);

        // get size of all staging resources
        size_t getStagingUsage() const { return m_StagingResourcesSize; }

        // returns staging buffer information that can be used for any manual copy operations.
        // if data is non-null it will be copied to bufferMapping automatically
        // virtual so that derived classes can implement this by different means.
        virtual VkResult acquireStagingSpace(BufferRange& staging_space, size_t data_size, const void* data, const SemaphoreState& semaphore_state = {});

        // clear all pending copy operations.
        // this does NOT release resources of such operations and
        // `releaseStaging` is still required.
        void cancelAppended();

        // check if no operations were appended
        bool isAppendedEmpty() const;

        // start operations that are only on transfer queue
        // state is reset on `cancelAppended` or `cmdUploadAppended`
        void beginTransferOnly();

        // buffer.buffer, buffer.bufferSize and buffer.mapping are used
        // if buffer.mapping is valid, then we directly write to it
        // else staging space is acquired and a copy command appended
        // for later execution via `cmdUploadAppended`.
        // `dataSize` can be `0` does return VK_SUCCESS
        VkResult appendBuffer(const vk_test::Buffer& buffer,
                              VkDeviceSize           buffer_offset,
                              VkDeviceSize           data_size,
                              const void*            data,
                              const SemaphoreState&  semaphore_state = {});

        // same as above but infers `bufferOffset` from `bufferRange.offset` and `dataSize` from `bufferRange.range`
        VkResult appendBufferRange(const vk_test::BufferRange& buffer_range, const void* data, const SemaphoreState& semaphore_state = {});

        // buffer.buffer, buffer.bufferSize and buffer.mapping are used
        // if buffer.mapping is valid, then we return it as `uploadMapping`
        // else staging space is acquired its mapping is returned in `uploadMapping`
        // and a copy command is appended for later execution via `cmdUploadAppended`
        // `dataSize` can be `0` does return VK_SUCCESS and sets `uploadMapping` to nullptr
        VkResult appendBufferMapping(const vk_test::Buffer& buffer,
                                     VkDeviceSize           bufferOffset,
                                     VkDeviceSize           dataSize,
                                     void*&                 uploadMapping,
                                     const SemaphoreState&  semaphore_state = {});

        // same as above but infers `bufferOffset` from `bufferRange.offset` and `dataSize` from `bufferRange.range`
        VkResult appendBufferRangeMapping(const vk_test::BufferRange& bufferRange, void*& uploadMapping, const SemaphoreState& semaphore_state = {});

        template <typename T>
        VkResult appendBuffer(const vk_test::Buffer& buffer, size_t buffer_offset, std::span<T> data, const SemaphoreState& semaphore_state = {}) {
            return appendBuffer(buffer, buffer_offset, data.size_bytes(), data.data(), semaphore_state);
        }

        template <typename T>
        VkResult appendBufferMapping(const vk_test::Buffer& buffer,
                                     size_t                 buffer_offset,
                                     size_t                 data_size,
                                     T*&                    upload_mapping,
                                     const SemaphoreState&  semaphore_state = {}) {
            return appendBufferMapping(buffer, buffer_offset, data_size, (void*&) upload_mapping, semaphore_state);
        }

        template <typename T>
        VkResult appendBufferRange(const vk_test::BufferRange& buffer_range, std::span<T> data, const SemaphoreState& semaphore_state = {}) {
            assert(buffer_range.range == data.size_bytes());
            return appendBufferRange(buffer_range, data.data(), semaphore_state);
        }

        template <typename T>
        VkResult appendBufferRangeMapping(const vk_test::BufferRange& buffer_range,
                                          T*&                         upload_mapping,
                                          const SemaphoreState&       semaphore_state = {}) {
            return appendBufferRangeMapping(buffer_range, (void*&) upload_mapping, semaphore_state);
        }

        // if the internal state StagingUploader's `enableLayoutBarriers` is true
        // then all appendImage functions may add barriers prior and after
        // the copy operations. These barriers are added
        // if `imageTex.descriptor.imageLayout` is not (VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or
        //                                              VK_IMAGE_LAYOUT_GENERAL or
        //                                              VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR)
        // then we transition to `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` prior any copy operation in
        // the appended batch. After the copy operations a transition is added back to the image's
        // original layout unless a valid `newLayout` is provided.
        // The `imageTex.descriptor.imageLayout` can also be updated
        // by providing `newLayout != VK_IMAGE_LAYOUT_UNDEFINED` through the post operation barriers.

        // all image textures must only use the color aspect
        // this uploads mip 0/layer 0 only
        VkResult appendImage(vk_test::Image&       image,
                             size_t                data_size,
                             const void*           data,
                             VkImageLayout         new_layout      = VK_IMAGE_LAYOUT_UNDEFINED,
                             const SemaphoreState& semaphore_state = {});

        template <typename T>
        VkResult appendImage(vk_test::Image&       image,
                             std::span<T>          data,
                             VkImageLayout         new_layout      = VK_IMAGE_LAYOUT_UNDEFINED,
                             const SemaphoreState& semaphore_state = {}) {
            return appendImage(image, data.size_bytes(), data.data(), new_layout, semaphore_state);
        }

        // subresource variant
        VkResult appendImageSub(vk_test::Image&                 image,
                                const VkOffset3D&               offset,
                                const VkExtent3D&               extent,
                                const VkImageSubresourceLayers& subresource,
                                size_t                          data_size,
                                const void*                     data,
                                VkImageLayout                   new_layout      = VK_IMAGE_LAYOUT_UNDEFINED,
                                const SemaphoreState&           semaphore_state = {});

        template <typename T>
        VkResult appendImageSub(vk_test::Image&                 image,
                                const VkOffset3D&               offset,
                                const VkExtent3D&               extent,
                                const VkImageSubresourceLayers& subresource,
                                std::span<T>                    data,
                                VkImageLayout                   new_layout      = VK_IMAGE_LAYOUT_UNDEFINED,
                                const SemaphoreState&           semaphore_state = {}) {
            return appendImageSub(image, offset, extent, subresource, data.size_bytes(), data.data(), new_layout, semaphore_state);
        }

        // returns true if the sum of staging resources used in pending operations
        // and the added size is beyond the limit
        bool checkAppendedSize(size_t limit_in_bytes, size_t added_size = 0) const;

        // records pending operations (copy & relevant layout transitions) into the command buffer
        // and then resets the internal state for appended.
        void cmdUploadAppended(VkCommandBuffer cmd);

    protected:
        void modifyImageBarrier(VkImageMemoryBarrier2& barrier) const;

        struct Batch {
            bool   transfer_only = false;
            size_t staging_size  = 0;

            std::vector<VkBufferCopy2>            copy_buffer_regions;
            std::vector<VkCopyBufferInfo2>        copy_buffer_infos;
            std::vector<VkBufferImageCopy2>       copy_buffer_image_regions;
            std::vector<VkCopyBufferToImageInfo2> copy_buffer_image_infos;
            BarrierContainer                      pre;
            BarrierContainer                      post;
        };

        struct StagingResource {
            vk_test::Buffer buffer;
            SemaphoreState  semaphore_state;
        };

        ResourceAllocator* m_ResourceAllocator    = nullptr;
        size_t             m_StagingResourcesSize = 0;
        bool               m_EnableLayoutBarriers = false;

        std::vector<StagingResource> m_StagingResources;
        Batch                        m_Batch{};
    };

} // namespace vk_test
