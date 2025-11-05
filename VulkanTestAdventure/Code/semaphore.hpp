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

#include "Resources.hpp"

namespace vk_test {

    // simple wrapper to create a timeline semaphore
    VkResult createTimelineSemaphore(VkDevice device, uint64_t initial_value, VkSemaphore& semaphore);

    // The SemaphoreState class wraps a timeline semaphore
    // with a timeline value.
    //
    // It can be only one of two states:
    //   - fixed: the timeline value is fixed and cannot be changed
    //   - dynamic: the timeline value is provided at a later time, exactly once
    //
    // The latter usecase is intended in conjunction with the `nvvk::QueueTimeline` class.
    // Any semaphore state that is signalled within `nvvk::QueueTimeline::submit(...)` that
    // was created from that `nvvk::QueueTimeline` will have its timeline value updated at that time.
    //
    // In both cases a copy of the struct can be made to later check the completion status of
    // the timeline semaphore.

    class SemaphoreState {
    public:
        static SemaphoreState makeFixed(VkSemaphore semaphore, uint64_t timeline_value) {
            SemaphoreState sem_state;
            sem_state.initFixed(semaphore, timeline_value);
            return sem_state;
        }

        static SemaphoreState makeFixed(const SemaphoreInfo& semaphore_info) {
            SemaphoreState sem_state;
            sem_state.initFixed(semaphore_info.semaphore, semaphore_info.value);
            return sem_state;
        }

        static SemaphoreState makeDynamic(VkSemaphore semaphore) {
            SemaphoreState sem_state;
            sem_state.initDynamic(semaphore);
            return sem_state;
        }

        SemaphoreState() {}

        void initFixed(VkSemaphore semaphore, uint64_t timeline_value) {
            assert(m_semaphore == nullptr);
            assert(timeline_value && semaphore);
            m_semaphore    = semaphore;
            m_fixedValue   = timeline_value;
            m_dynamicValue = nullptr;
        }

        void initDynamic(VkSemaphore semaphore) {
            assert(m_semaphore == nullptr);
            assert(semaphore);
            m_semaphore    = semaphore;
            m_fixedValue   = 0;
            m_dynamicValue = std::make_shared<std::atomic_uint64_t>(0);
        }

        bool isValid() const { return (m_semaphore != nullptr) && (m_fixedValue != 0 || m_dynamicValue); }
        bool isFixed() const { return (m_semaphore != nullptr) && (m_fixedValue != 0); }
        bool isDynamic() const { return (m_semaphore != nullptr) && (m_dynamicValue); }

        VkSemaphore getSemaphore() const { return m_semaphore; }
        uint64_t    getTimelineValue() const {
            if (m_fixedValue != 0U) {
                return m_fixedValue;
            }
            if (m_dynamicValue) {
                return m_dynamicValue->load();
            }

            return 0;
        }

        // this function can be called only once and is only legal for
        // dynamic semaphore state
        void setDynamicValue(uint64_t value) {
            // must be dynamic, and must not have been set already
            assert(isDynamic() && m_dynamicValue->load() == 0);
            // updated the shared_ptr value so every copy of this
            // semaphore state has access to it.
            m_dynamicValue->store(value);

            // fixate afterwards to update local cache
            // and decouple it
            fixate();
        }

        // for dynamic values waiting and testing will always return false
        // if the m_dynamicValue->load() returns zero, meaning
        // the SemaphoreState wasn't part of a `QueueTimeline::submit` where it was
        // signaled yet.
        //
        // non-const versions implicitly try to fixate the value in the dynamic
        // case to speed things up a bit for future waits or tests.

        VkResult wait(VkDevice device, uint64_t timeout) const;
        VkResult wait(VkDevice device, uint64_t timeout);

        bool testSignaled(VkDevice device) const;
        bool testSignaled(VkDevice device);

        bool canWait() const {
            return (m_semaphore != nullptr) && (m_fixedValue != 0 || (m_dynamicValue && m_dynamicValue->load() != 0));
        }
        bool canWait() {
            fixate();
            return static_cast<const SemaphoreState*>(this)->canWait();
        }

    private:
        // attempts to convert dynamic to fixed value if possible,
        // can speed up future waits.
        void fixate();

        VkSemaphore m_semaphore{};

        // Holds the timeline value of a semaphore.
        //
        // We are using a shared_ptr as this struct can be used to encode
        // future submits, and therefore the actual value of the timeline semaphore's
        // submission isn't known yet.
        //
        // The shared_ptr will point towards the location that will contain the final value.
        // By default the value will be 0 (not-submitted).

        // doesn't exist for "fixed" value semaphore state
        std::shared_ptr<std::atomic_uint64_t> m_dynamicValue;

        // stores either the fixed value, or is updated to have
        // a local cache of the dynamic value.
        // By design a dynamic value can only once be changed from 0 to its real value.
        uint64_t m_fixedValue{};
    };

    struct SemaphoreSubmitState {
        SemaphoreState           semaphoreState;
        VkPipelineStageFlagBits2 stageMask   = 0;
        uint32_t                 deviceIndex = 0;
    };

    inline VkSemaphoreSubmitInfo makeSemaphoreSubmitInfo(const SemaphoreState&    semaphore_state,
                                                         VkPipelineStageFlagBits2 stage_mask,
                                                         uint32_t                 device_index = 0) {
        assert(semaphore_state.isValid());

        VkSemaphoreSubmitInfo semaphore_submit_info = {
            .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore   = semaphore_state.getSemaphore(),
            .value       = semaphore_state.getTimelineValue(),
            .stageMask   = stage_mask,
            .deviceIndex = device_index,
        };

        // assert proper timeline value has been set
        assert(semaphore_submit_info.value && "semaphore state has invalid timelineValue");

        return semaphore_submit_info;
    };

    inline VkSemaphoreSubmitInfo makeSemaphoreSubmitInfo(const SemaphoreSubmitState& state) {
        assert(state.semaphoreState.isValid());

        VkSemaphoreSubmitInfo semaphore_submit_info = {
            .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore   = state.semaphoreState.getSemaphore(),
            .value       = state.semaphoreState.getTimelineValue(),
            .stageMask   = state.stageMask,
            .deviceIndex = state.deviceIndex,
        };

        // assert proper timeline value has been set
        assert(semaphore_submit_info.value && "semaphore state has invalid timelineValue");

        return semaphore_submit_info;
    };

} // namespace vk_test
