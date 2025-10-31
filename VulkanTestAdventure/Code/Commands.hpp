#pragma once

namespace vk_test {
    // Helper to create a transient command pool
    // Transient command pools are meant to be used for short-lived commands.
    VkCommandPool createTransientCommandPool(VkDevice device, uint32_t queue_family_index);

    // Simple helper for the creation of a temporary command buffer, use to record the commands to upload data, or transition images.
    // Submit the temporary command buffer, wait until the command is finished, and clean up.
    // Allocates a single shot command buffer from the provided pool and begins it.
    VkResult beginSingleTimeCommands(VkCommandBuffer& command, VkDevice device, VkCommandPool command_pool);

    inline VkCommandBuffer createSingleTimeCommands(VkDevice device, VkCommandPool command_pool) {
        VkCommandBuffer cmd{};
        beginSingleTimeCommands(cmd, device, command_pool);
        return cmd;
    }

    // Ends command buffer, submits on the provided queue, then waits for completion, and frees the command buffer within the provided pool
    VkResult endSingleTimeCommands(VkCommandBuffer command, VkDevice device, VkCommandPool command_pool, VkQueue queue);
} // namespace vk_test
