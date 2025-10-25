#pragma once

namespace vk_test {
    struct QueueInfo {
        uint32_t familyIndex = ~0U; // Family index of the queue (graphic, compute, transfer, ...)
        uint32_t queueIndex  = ~0U; // Index of the queue in the family
        VkQueue  queue{};           // The queue object
    };

}; // namespace vk_test
