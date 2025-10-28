#include "pch.h"
#include "Commands.hpp"

VkCommandPool createTransientCommandPool(VkDevice device, uint32_t queueFamilyIndex) {
    VkCommandPool                 cmdPool = VK_NULL_HANDLE;
    const VkCommandPoolCreateInfo commandPoolCreateInfo{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, // Hint that commands will be short-lived
        .queueFamilyIndex = queueFamilyIndex,
    };
    vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &cmdPool);
    return cmdPool;
}

VkResult beginSingleTimeCommands(VkCommandBuffer& cmd, VkDevice device, VkCommandPool cmdPool) {
    const VkCommandBufferAllocateInfo allocInfo{ .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                                 .commandPool        = cmdPool,
                                                 .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                 .commandBufferCount = 1 };
    vkAllocateCommandBuffers(device, &allocInfo, &cmd);
    const VkCommandBufferBeginInfo beginInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                              .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    vkBeginCommandBuffer(cmd, &beginInfo);
    return VK_SUCCESS;
}

VkResult endSingleTimeCommands(VkCommandBuffer cmd, VkDevice device, VkCommandPool cmdPool, VkQueue queue) {
    // Submit and clean up
    vkEndCommandBuffer(cmd);

    // Create fence for synchronization
    const VkFenceCreateInfo fenceInfo{ .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    std::array<VkFence, 1>  fence{};
    vkCreateFence(device, &fenceInfo, nullptr, fence.data());

    const VkCommandBufferSubmitInfo    cmdBufferInfo{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = cmd };
    const std::array<VkSubmitInfo2, 1> submitInfo{
        { { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .commandBufferInfoCount = 1, .pCommandBufferInfos = &cmdBufferInfo } }
    };
    vkQueueSubmit2(queue, uint32_t(submitInfo.size()), submitInfo.data(), fence[0]);
    vkWaitForFences(device, uint32_t(fence.size()), fence.data(), VK_TRUE, UINT64_MAX);

    // Cleanup
    vkDestroyFence(device, fence[0], nullptr);
    vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
    return VK_SUCCESS;
}
