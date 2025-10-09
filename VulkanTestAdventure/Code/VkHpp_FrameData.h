#pragma once

namespace VKHppTest {
    struct FrameData {
        vk::raii::CommandBuffer command_buffer = VK_NULL_HANDLE;

        vk::raii::Semaphore image_available_semaphore = VK_NULL_HANDLE;
        vk::raii::Semaphore render_finished_semaphore = VK_NULL_HANDLE;
        vk::raii::Fence     in_flight_fence           = VK_NULL_HANDLE;

        vk::raii::Buffer       scene_uniform_buffer        = VK_NULL_HANDLE; // optional
        vk::raii::DeviceMemory scene_uniform_buffer_memory = VK_NULL_HANDLE; // optional
        void*                  scene_uniform_buffer_mapped = nullptr;        // optional

        FrameData()  = default;
        ~FrameData() = default;
    };
} // namespace VKHppTest
