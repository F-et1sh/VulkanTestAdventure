#pragma once

#define VK_TEST_SAY(message) std::wcerr << std::endl \
                                        << message << std::endl
#define VK_TEST_SAY_WITHOUT_BREAK(message) std::wcerr << message

#define VK_TEST_RUNTIME_ERROR(message) throw std::runtime_error(message)

#define VK_TEST_NODISCARD [[nodiscard]]

namespace vk_test {
    VkCommandPool createTransientCommandPool(VkDevice device, uint32_t queue_family_index) {
        VkCommandPool                 cmd_pool = VK_NULL_HANDLE;
        const VkCommandPoolCreateInfo command_pool_create_info{
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, // Hint that commands will be short-lived
            .queueFamilyIndex = queue_family_index,
        };
        vkCreateCommandPool(device, &command_pool_create_info, nullptr, &cmd_pool);
        return cmd_pool;
    }

    VkResult beginSingleTimeCommands(VkCommandBuffer& cmd, VkDevice device, VkCommandPool cmd_pool) {
        const VkCommandBufferAllocateInfo alloc_info{
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = cmd_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };
        vkAllocateCommandBuffers(device, &alloc_info, &cmd);
        const VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };
        vkBeginCommandBuffer(cmd, &begin_info);
        return VK_SUCCESS;
    }

    VkResult endSingleTimeCommands(VkCommandBuffer cmd, VkDevice device, VkCommandPool cmd_pool, VkQueue queue) {
        // Submit and clean up
        vkEndCommandBuffer(cmd);

        // Create fence for synchronization
        const VkFenceCreateInfo fence_info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
        };
        std::array<VkFence, 1> fence{};
        vkCreateFence(device, &fence_info, nullptr, fence.data());

        const VkCommandBufferSubmitInfo cmd_buffer_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = cmd
        };
        const std::array<VkSubmitInfo2, 1> submit_info{
            { { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .commandBufferInfoCount = 1, .pCommandBufferInfos = &cmd_buffer_info } }
        };
        vkQueueSubmit2(queue, uint32_t(submit_info.size()), submit_info.data(), fence[0]);
        vkWaitForFences(device, uint32_t(fence.size()), fence.data(), VK_TRUE, UINT64_MAX);

        // Cleanup
        vkDestroyFence(device, fence[0], nullptr);
        vkFreeCommandBuffers(device, cmd_pool, 1, &cmd);
        return VK_SUCCESS;
    }

    inline static void copy_file(const std::filesystem::path& from, const std::filesystem::path& to) {
        if (!std::filesystem::exists(to)) {
            std::filesystem::create_directories(to);
        }
        for (const auto& entry : std::filesystem::recursive_directory_iterator(from)) {
            const auto& dest_path = to / std::filesystem::relative(entry.path(), from);
            if (std::filesystem::is_directory(entry.status())) {
                std::filesystem::create_directories(dest_path);
            }
            else if (std::filesystem::is_regular_file(entry.status())) {
                std::filesystem::copy_file(entry.path(), dest_path, std::filesystem::copy_options::overwrite_existing);
            }
        }
    }

    inline static void copy_if_new(const std::filesystem::path& from, const std::filesystem::path& to) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(from)) {
            auto relative_path = std::filesystem::relative(entry.path(), from);
            auto target_path   = to / relative_path;

            if (entry.is_directory()) {
                std::filesystem::create_directories(target_path);
            }
            else if (!std::filesystem::exists(target_path) ||
                     std::filesystem::last_write_time(entry) > std::filesystem::last_write_time(target_path)) {
                std::filesystem::copy_file(entry, target_path, std::filesystem::copy_options::overwrite_existing);
            }
        }
    }

    inline static auto generate_unique_filename(const std::filesystem::path& file_path) -> std::filesystem::path {
        std::filesystem::path path      = file_path;
        std::filesystem::path stem      = path.stem();
        std::filesystem::path extension = path.extension();
        int                   count     = 1;

        while (std::filesystem::exists(path)) {
            path.replace_filename(stem.wstring() + L" (" + std::to_wstring(count) + L")" + extension.wstring());
            count++;
        }

        return path;
    }
} // namespace vk_test
