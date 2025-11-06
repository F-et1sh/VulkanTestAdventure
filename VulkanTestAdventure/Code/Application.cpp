#include "pch.h"
#include "Application.hpp"

void vk_test::Application::Release() {
    vkDeviceWaitIdle(m_Device);

    // Clean pending
    resetFreeQueue(0);

    m_Swapchain.Release();

    // Frame info
    for (size_t i = 0; i < m_FrameData.size(); i++) {
        vkFreeCommandBuffers(m_Device, m_FrameData[i].command_pool, 1, &m_FrameData[i].command_buffer);
        vkDestroyCommandPool(m_Device, m_FrameData[i].command_pool, nullptr);
    }
    vkDestroySemaphore(m_Device, m_FrameTimelineSemaphore, nullptr);

    vkDestroyCommandPool(m_Device, m_TransientCommandPool, nullptr);
    vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);

    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
}

void vk_test::Application::Initialize(ApplicationCreateInfo& info) {
    m_Instance       = info.instance;
    m_Device         = info.device;
    m_PhysicalDevice = info.physical_device;
    m_Queues         = info.queues;
    m_MaxTexturePool = info.texture_pool_size;

    // Set the default size and position of the window
    testAndSetWindowSizeAndPos({ info.window_size.x, info.window_size.y });

    // Used for creating single-time command buffers
    createTransientCommandPool();

    // Create a descriptor pool for creating descriptor set in the application
    createDescriptorPool();

    // VK_ERROR_EXTENSION_NOT_PRESENT
    const VkResult check_result = glfwCreateWindowSurface(m_Instance, m_Window.getGLFWWindow(), nullptr, &m_Surface);

    // Create the swapchain
    Swapchain::InitInfo swapchain_init{
        .physical_device          = m_PhysicalDevice,
        .device                   = m_Device,
        .queue                    = m_Queues[0],
        .surface                  = m_Surface,
        .command_pool             = m_TransientCommandPool,
        .preferred_vsync_off_mode = info.preferred_vsync_off_mode,
        .preferred_vsync_on_mode  = info.preferred_vsync_on_mode,
    };

    m_Swapchain.Initialize(swapchain_init);
    m_Swapchain.InitializeResources(m_WindowExtent, IS_VSYN_WANTED); // Update the window size to the actual size of the surface

    // Create what is needed to submit the scene for each frame in-flight
    createFrameSubmission(m_Swapchain.getMaxFramesInFlight());

    // Set up the resource free queue
    resetFreeQueue(getFrameCycleSize());
}

void vk_test::Application::Loop() {
    while (glfwWindowShouldClose(m_Window.getGLFWWindow()) == 0) {

        //// Frame Resource Preparation
        //if (prepareFrameResources()) {
        //    // Free resources from previous frame
        //    freeResourcesQueue();

        //    // Prepare Frame Synchronization
        //    prepareFrameToSignal(m_swapchain.getMaxFramesInFlight());

        //    // Record Commands
        //    VkCommandBuffer cmd = beginCommandRecording();
        //    drawFrame(cmd);           // Call onUIRender() and onRender() for each element
        //    renderToSwapchain(cmd);   // Render ImGui to swapchain
        //    addSwapchainSemaphores(); // Setup synchronization
        //    endFrame(cmd, m_swapchain.getMaxFramesInFlight());

        //    // Present Frame
        //    presentFrame(); // This can also trigger swapchain rebuild

        //    // Advance Frame
        //    advanceFrame(m_swapchain.getMaxFramesInFlight());
        //}

        vk_test::Window::PollEvents();
    }
}

VkCommandBuffer vk_test::Application::createTempCmdBuffer() const {
    VkCommandBuffer cmd{};
    beginSingleTimeCommands(cmd, m_Device, m_TransientCommandPool);
    return cmd;
}

void vk_test::Application::submitAndWaitTempCmdBuffer(VkCommandBuffer cmd) {
    endSingleTimeCommands(cmd, m_Device, m_TransientCommandPool, m_Queues[0].queue);
}

//-----------------------------------------------------------------------
// Create a command pool for short lived operations
// The command pool is used to allocate command buffers.
// In the case of this sample, we only need one command buffer, for temporary execution.
//
void vk_test::Application::createTransientCommandPool() {
    const VkCommandPoolCreateInfo command_pool_create_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, // Hint that commands will be short-lived
        .queueFamilyIndex = m_Queues[0].family_index,
    };
    vkCreateCommandPool(m_Device, &command_pool_create_info, nullptr, &m_TransientCommandPool);
}

//-----------------------------------------------------------------------
// Creates a command pool (long life) and buffer for each frame in flight. Unlike the temporary command pool,
// these pools persist between frames and don't use VK_COMMAND_POOL_CREATE_TRANSIENT_BIT.
// Each frame gets its own command buffer which records all rendering commands for that frame.
//
void vk_test::Application::createFrameSubmission(uint32_t num_frames) {
    assert(num_frames >= 2); // Must have at least 2 frames in flight

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
    const VkCommandPoolCreateInfo cmd_pool_create_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = m_Queues[0].family_index,
    };

    for (uint32_t i = 0; i < num_frames; i++) {
        m_FrameData[i].frame_number = i; // Track frame index for synchronization

        // Separate pools allow independent reset/recording of commands while other frames are still in-flight
        vkCreateCommandPool(m_Device, &cmd_pool_create_info, nullptr, &m_FrameData[i].command_pool);

        const VkCommandBufferAllocateInfo command_buffer_allocate_info = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = m_FrameData[i].command_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        vkAllocateCommandBuffers(m_Device, &command_buffer_allocate_info, &m_FrameData[i].command_buffer);
    }
}

void vk_test::Application::resetFreeQueue(uint32_t size) {
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

//-----------------------------------------------------------------------
// The Descriptor Pool is used to allocate descriptor sets.
// Currently, ImGui only requires combined image samplers.
// We allocate up to m_maxTexturePool of them so that we can display additional
// images using ImGui_ImplVulkan_AddTexture().
//
void vk_test::Application::createDescriptorPool() {
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

void vk_test::Application::testAndSetWindowSizeAndPos(const glm::uvec2& window_size) {
    bool center_window = false;
    // If window_size is provided, use it
    if (window_size.x != 0 && window_size.y != 0) {
        m_WindowSize  = window_size;
        center_window = true; // When the window size is requested, it will be centered
    }

    // If m_WindowSize is still (0,0), set defaults
    // Could be not zero if the user set it in the settings (see loading of the ini file)
    if (m_WindowSize.x == 0 && m_WindowSize.y == 0) {
        {
            // Get 80% of primary monitor
            const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
            m_WindowSize.x          = static_cast<int>(mode->width * 0.8F);
            m_WindowSize.y          = static_cast<int>(mode->height * 0.8F);
        }
        // Center the window
        int mon_x = 0;
        int mon_y = 0;
        glfwGetMonitorPos(glfwGetPrimaryMonitor(), &mon_x, &mon_y);
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        m_WindowPosition.x      = mon_x + (mode->width - m_WindowSize.x) / 2;
        m_WindowPosition.y      = mon_y + (mode->height - m_WindowSize.y) / 2;
    }
    else {
        // If m_winPos was retrieved, check if it is valid
        if (!isWindowPosValid(m_WindowPosition) || center_window) {
            // Center the window
            int mon_x = 0;
            int mon_y = 0;
            glfwGetMonitorPos(glfwGetPrimaryMonitor(), &mon_x, &mon_y);
            const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
            m_WindowPosition.x      = mon_x + (mode->width - m_WindowSize.x) / 2;
            m_WindowPosition.y      = mon_y + (mode->height - m_WindowSize.y) / 2;
        }
    }

    m_WindowExtent = { m_WindowSize.x, m_WindowSize.y };
}

// Check if window position is within visible monitor bounds
bool vk_test::Application::isWindowPosValid(const glm::ivec2& window_position) {
    int           monitor_count = 0;
    GLFWmonitor** monitors      = glfwGetMonitors(&monitor_count);

    // For each connected monitor
    for (int i = 0; i < monitor_count; i++) {
        GLFWmonitor*       monitor = monitors[i];
        const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

        int mon_x = 0;
        int mon_y = 0;
        glfwGetMonitorPos(monitor, &mon_x, &mon_y);

        // Check if window position is within this monitor's bounds
        // Add some margin to account for window decorations
        if (window_position.x >= mon_x &&
            window_position.x < mon_x + mode->width &&
            window_position.y >= mon_y &&
            window_position.y < mon_y + mode->height) {
            return true;
        }
    }

    return false;
}
