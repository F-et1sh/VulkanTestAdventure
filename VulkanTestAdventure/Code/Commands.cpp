#include "pch.h"
#include "Commands.hpp"

namespace vk_test {
    VkCommandPool createTransientCommandPool(VkDevice device, uint32_t queue_family_index) {
        VkCommandPool                 command_pool = VK_NULL_HANDLE;
        const VkCommandPoolCreateInfo command_pool_create_info{
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, // Hint that commands will be short-lived
            .queueFamilyIndex = queue_family_index,
        };
        vkCreateCommandPool(device, &command_pool_create_info, nullptr, &command_pool);
        return command_pool;
    }

    VkResult beginSingleTimeCommands(VkCommandBuffer& command, VkDevice device, VkCommandPool command_pool) {
        const VkCommandBufferAllocateInfo alloc_info{ .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                      .commandPool        = command_pool,
                                                      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                      .commandBufferCount = 1 };
        vkAllocateCommandBuffers(device, &alloc_info, &command);
        const VkCommandBufferBeginInfo begin_info{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                                   .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
        vkBeginCommandBuffer(command, &begin_info);
        return VK_SUCCESS;
    }

    VkResult endSingleTimeCommands(VkCommandBuffer command, VkDevice device, VkCommandPool command_pool, VkQueue queue) {
        // Submit and clean up
        vkEndCommandBuffer(command);

        // Create fence for synchronization
        const VkFenceCreateInfo fence_info{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        std::array<VkFence, 1>  fence{};
        vkCreateFence(device, &fence_info, nullptr, fence.data());

        const VkCommandBufferSubmitInfo    cmd_buffer_info{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = command };
        const std::array<VkSubmitInfo2, 1> submit_info{
            { { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .commandBufferInfoCount = 1, .pCommandBufferInfos = &cmd_buffer_info } }
        };
        vkQueueSubmit2(queue, uint32_t(submit_info.size()), submit_info.data(), fence[0]);
        vkWaitForFences(device, uint32_t(fence.size()), fence.data(), VK_TRUE, UINT64_MAX);

        // Cleanup
        vkDestroyFence(device, fence[0], nullptr);
        vkFreeCommandBuffers(device, command_pool, 1, &command);
        return VK_SUCCESS;
    }

} // namespace vk_test
