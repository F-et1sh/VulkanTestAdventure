#pragma once

namespace vk_test {
    struct QueueInfo {
        uint32_t family_index = ~0U; // Family index of the queue (graphic, compute, transfer, ...)
        uint32_t queue_index  = ~0U; // Index of the queue in the family
        VkQueue  queue{};           // The queue object
    };

    struct SemaphoreInfo {
        VkSemaphore semaphore{}; // Timeline semaphore
        uint64_t    value{};     // Timeline value
    };

} // namespace vk_test
