#include "pch.h"
#include "RendererVulkan.hpp"

void vk_test::RendererVulkan::Release() {
    vkDeviceWaitIdle(m_Device);

    // Clean pending
    resetFreeQueue(0);

    //m_Swapchain.Release();

    // Frame info
    for (size_t i = 0; i < m_FrameData.size(); i++) {
        vkFreeCommandBuffers(m_Device, m_FrameData[i].command_pool, 1, &m_FrameData[i].command_buffer);
        vkDestroyCommandPool(m_Device, m_FrameData[i].command_pool, nullptr);
    }
    vkDestroySemaphore(m_Device, m_FrameTimelineSemaphore, nullptr);

    vkDestroyCommandPool(m_Device, m_TransientCmdPool, nullptr);
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
}

void vk_test::RendererVulkan::Initialize(RendererVulkanCreateInfo& info) {
    m_Instance       = info.instance;
    m_Device         = info.device;
    m_PhysicalDevice = info.physical_device;
    m_Queues         = info.queues;
    m_ViewportSize   = {}; // Will be set by the first viewport size
    m_MaxTexturePool = info.texture_pool_size;

    // Used for creating single-time command buffers
    createTransientCommandPool();

    // Create a descriptor pool for creating descriptor set in the application
    createDescriptorPool();

    // Create the swapchain
    Swapchain::InitInfo swapchain_init{
        .physical_device          = m_PhysicalDevice,
        .device                   = m_Device,
        .queue                    = m_Queues[0],
        .surface                  = m_Surface,
        .command_pool             = m_TransientCmdPool,
        .preferred_vsync_off_mode = info.preferred_vsync_off_mode,
        .preferred_vsync_on_mode  = info.preferred_vsync_on_mode,
    };

    m_Swapchain.init(swapchain_init);
    m_Swapchain.initResources(m_WindowSize, false); // Update the window size to the actual size of the surface

    // Create what is needed to submit the scene for each frame in-flight
    createFrameSubmission(m_Swapchain.getMaxFramesInFlight());

    // Set up the resource free queue
    resetFreeQueue(getFrameCycleSize());
}

void vk_test::RendererVulkan::createTransientCommandPool() {
    const VkCommandPoolCreateInfo command_pool_create_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, // Hint that commands will be short-lived
        .queueFamilyIndex = m_Queues[0].family_index,
    };
    vkCreateCommandPool(m_Device, &command_pool_create_info, nullptr, &m_TransientCmdPool);
}

void vk_test::RendererVulkan::createDescriptorPool() {
    const std::array<VkDescriptorPoolSize, 1> pool_sizes{
        { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_MaxTexturePool } },
    };

    const VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT |  //  allows descriptor sets to be updated after they have been bound to a command buffer
                 VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, // individual descriptor sets can be freed from the descriptor pool
        .maxSets       = m_MaxTexturePool,                          // Allowing to create many sets (ImGui uses this for textures)
        .poolSizeCount = uint32_t(pool_sizes.size()),
        .pPoolSizes    = pool_sizes.data(),
    };
    vkCreateDescriptorPool(m_Device, &pool_info, nullptr, &m_DescriptorPool);
}

void vk_test::RendererVulkan::createFrameSubmission(uint32_t num_frames) {
    assert(numFrames >= 2); // Must have at least 2 frames in flight

    m_FrameData.resize(num_frames);

    // Initialize timeline semaphore with (numFrames - 1) to allow concurrent frame submission. See details in README.md
    const uint64_t initial_value = (static_cast<uint64_t>(num_frames) - 1);

    VkSemaphoreTypeCreateInfo timeline_create_info = {
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext         = nullptr,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue  = initial_value,
    };

    // Create timeline semaphore for GPU-CPU synchronization
    // This ensures resources aren't overwritten while still in use by the GPU
    const VkSemaphoreCreateInfo semaphore_create_info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = &timeline_create_info };
    vkCreateSemaphore(m_Device, &semaphore_create_info, nullptr, &m_FrameTimelineSemaphore);

    //Create command pools and buffers for each frame
    //Each frame gets its own command pool to allow parallel command recording while previous frames may still be executing on the GPU
    const VkCommandPoolCreateInfo command_pool_create_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = m_Queues[0].family_index,
    };

    for (uint32_t i = 0; i < num_frames; i++) {
        m_FrameData[i].frame_number = i; // Track frame index for synchronization

        // Separate pools allow independent reset/recording of commands while other frames are still in-flight
        vkCreateCommandPool(m_Device, &command_pool_create_info, nullptr, &m_FrameData[i].command_pool);

        const VkCommandBufferAllocateInfo command_buffer_allocate_info = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = m_FrameData[i].command_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        vkAllocateCommandBuffers(m_Device, &command_buffer_allocate_info, &m_FrameData[i].command_buffer);
    }
}

void vk_test::RendererVulkan::resetFreeQueue(uint32_t size) {
    vkDeviceWaitIdle(m_Device);

    for (auto& queue : m_ResourceFreeQueue) {
        // Free resources in queue
        for (auto& func : queue) {
            func();
        }
        queue.clear();
    }
    m_ResourceFreeQueue.clear();
    m_ResourceFreeQueue.resize(size);
}
