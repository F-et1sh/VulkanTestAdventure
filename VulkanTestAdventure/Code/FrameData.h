#pragma once

namespace VKTest {
    struct FrameData {
        vk::CommandBuffer command_buffer;

        vk::Semaphore image_available_semaphore;
        vk::Semaphore render_finished_semaphore;
        vk::Fence in_flight_fence;

        vk::Buffer uniform_buffer;
        vk::DeviceMemory uniform_buffer_memory;
        void* uniform_buffer_mapped = nullptr;

        FrameData() = default;
        ~FrameData() = default;
    };
}