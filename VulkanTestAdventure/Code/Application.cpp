#include "pch.h"
#include "Application.hpp"

void vk_test::Application::Release() {
    // This will call the onDetach of the elements
    for (std::shared_ptr<IAppElement>& e : m_Elements) {
        e->onDetach();
    }

    // Destroy the elements
    m_Elements.clear();

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
    VK_TEST_SAY("Running application");
    // Re-load ImGui settings from disk, as there might be application elements with settings to restore.
    //ImGui::LoadIniSettingsFromDisk(m_iniFilename.c_str());

    //// Handle headless mode
    //if (false) {
    //    headlessRun();
    //    return;
    //}

    // Main rendering loop
    while (!glfwWindowShouldClose(m_Window.getGLFWWindow())) {
        // Window System Events.
        // We add a delay before polling to reduce latency.
        //if (IS_VSYN_WANTED) {
        //m_FramePacer.pace();
        //}
        glfwPollEvents();

        // Skip rendering when minimized
        if (glfwGetWindowAttrib(m_Window.getGLFWWindow(), GLFW_ICONIFIED) == GLFW_TRUE) {
            //ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        // Begin New Frame for ImGui
        //ImGui_ImplVulkan_NewFrame();
        //ImGui_ImplGlfw_NewFrame();
        //ImGui::NewFrame();

        // Setup ImGui Docking and UI
        //setupImguiDock();
        //if (m_useMenubar && ImGui::BeginMainMenuBar()) {
        //for (std::shared_ptr<IAppElement>& e : m_elements) {
        //e->onUIMenu();
        //}
        //ImGui::EndMainMenuBar();
        //}

        // Handle Viewport Updates
        VkExtent2D viewport_extent = m_WindowExtent;
        //const ImGuiWindow* viewport     = ImGui::FindWindowByName("Viewport");
        //if (viewport) {
        //viewport_extent = { uint32_t(viewport->Size.x), uint32_t(viewport->Size.y) };
        //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        //ImGui::Begin("Viewport");
        //ImGui::End();
        //ImGui::PopStyleVar();
        //}

        // Update viewport if size changed
        if (m_ViewportExtent.width != viewport_extent.width || m_ViewportExtent.height != viewport_extent.height) {
            onViewportSizeChange(viewport_extent);
        }

        //// Handle Screenshot Requests
        //if (m_screenShotRequested && (m_FrameRingCurrent == m_screenShotFrame)) {
        //    saveScreenShot(m_screenShotFilename, k_imageQuality);
        //    m_screenShotRequested = false;
        //}

        // Frame Resource Preparation
        if (prepareFrameResources()) {
            // Free resources from previous frame
            freeResourcesQueue();

            // Prepare Frame Synchronization
            prepareFrameToSignal(m_Swapchain.getMaxFramesInFlight());

            // Record Commands
            VkCommandBuffer cmd = beginCommandRecording();
            drawFrame(cmd);           // Call onUIRender() and onRender() for each element
            renderToSwapchain(cmd);   // Render ImGui to swapchain
            addSwapchainSemaphores(); // Setup synchronization
            endFrame(cmd, m_Swapchain.getMaxFramesInFlight());

            // Present Frame
            presentFrame(); // This can also trigger swapchain rebuild

            // Advance Frame
            advanceFrame(m_Swapchain.getMaxFramesInFlight());
        }

        //// End ImGui frame
        //ImGui::EndFrame();

        //// Handle Additional ImGui Windows
        //if((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0)
        //{
        //  ImGui::UpdatePlatformWindows();
        //  ImGui::RenderPlatformWindowsDefault();
        //}
    }
}

void vk_test::Application::addElement(const std::shared_ptr<IAppElement>& layer) {
    m_Elements.emplace_back(layer);
    layer->onAttach(this);
}

VkCommandBuffer vk_test::Application::createTempCmdBuffer() const {
    VkCommandBuffer command{};
    beginSingleTimeCommands(command, m_Device, m_TransientCommandPool);
    return command;
}

void vk_test::Application::submitAndWaitTempCmdBuffer(VkCommandBuffer command) {
    endSingleTimeCommands(command, m_Device, m_TransientCommandPool, m_Queues[0].queue);
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

//-----------------------------------------------------------------------
// This is the headless version of the run loop.
// It will render the scene for the number of frames specified in the headlessFrameCount.
// It will call onUIRender() and onRender() for each element.
//
void vk_test::Application::headlessRun() {
    //ScopedTimer st(__FUNCTION__);
    m_ViewportExtent = m_WindowExtent;

    // Set the display for Imgui
    //ImGuiIO& io      = ImGui::GetIO();
    //io.DisplaySize.x = float(m_viewportSize.width);
    //io.DisplaySize.y = float(m_viewportSize.height);

    // Make the size has been communicated everywhere
    VkCommandBuffer cmd{};
    beginSingleTimeCommands(cmd, m_Device, m_TransientCommandPool);
    for (std::shared_ptr<IAppElement>& e : m_Elements) {
        e->onResize(cmd, m_ViewportExtent);
    }
    endSingleTimeCommands(cmd, m_Device, m_TransientCommandPool, m_Queues[0].queue);

    // Need to render the UI twice: the first pass sets up the internal state and layout,
    // and the second pass finalizes the rendering with the updated state.
    {
        //ImGui_ImplVulkan_NewFrame();
        //ImGui::NewFrame();
        //setupImguiDock();

        // Call UI rendering for each element
        for (std::shared_ptr<IAppElement>& e : m_Elements) {
            e->onUIRender();
        }
        //ImGui::EndFrame();
    }

    // Rendering n-times the scene
    for (uint32_t frameID = 0; frameID < 1; frameID++) {
        //ImGui_ImplVulkan_NewFrame();
        //ImGui::NewFrame(); // Even if isn't directly used, helps advancing time if query

        waitForFrameCompletion();

        prepareFrameToSignal(getFrameCycleSize());

        VkCommandBuffer cmd = beginCommandRecording(); // Start the command buffer
        drawFrame(cmd);                                // Call onUIRender() and onRender() for each element
        endFrame(cmd, getFrameCycleSize());            // End the frame and submit it
        advanceFrame(getFrameCycleSize());             // Advance to the next frame in the ring buffer

        //ImGui::EndFrame();
    }
    //ImGui::Render(); // This is creating the data to draw the UI (not on GPU yet)

    // At this point, everything has been rendered. Let it finish.
    vkDeviceWaitIdle(m_Device);

    // Call back the application, such that it can do something with the rendered image
    for (std::shared_ptr<IAppElement>& e : m_Elements) {
        e->onLastHeadlessFrame();
    }
}

//-----------------------------------------------------------------------
// Call this function if the viewport size changes
// This happens when the window is resized, or when the ImGui viewport window is resized.
//
void vk_test::Application::onViewportSizeChange(VkExtent2D extent) {
    // Check for DPI scaling and adjust the font size
    float xscale, yscale;
    glfwGetWindowContentScale(m_Window.getGLFWWindow(), &xscale, &yscale);
    //ImGui::GetIO().FontGlobalScale *= xscale / m_dpiScale;
    //m_dpiScale = xscale;

    m_ViewportExtent = extent;
    // Recreate the G-Buffer to the size of the viewport
    vkQueueWaitIdle(m_Queues[0].queue);
    {
        VkCommandBuffer cmd{};
        beginSingleTimeCommands(cmd, m_Device, m_TransientCommandPool);
        // Call the implementation of the UI rendering
        for (std::shared_ptr<IAppElement>& e : m_Elements) {
            e->onResize(cmd, m_ViewportExtent);
        }
        endSingleTimeCommands(cmd, m_Device, m_TransientCommandPool, m_Queues[0].queue);
    }
}

//-----------------------------------------------------------------------
// prepareFrameResources is the first step in the rendering process.
// It looks if the swapchain require rebuild, which happens when the window is resized.
// It acquires the image from the swapchain to render into.
///
bool vk_test::Application::prepareFrameResources() {
    if (m_Swapchain.needRebuilding()) {
        m_Swapchain.ReinitializeResources(m_WindowExtent, IS_VSYN_WANTED);
    }

    waitForFrameCompletion(); // Wait until GPU has finished processing

    VkResult result = m_Swapchain.acquireNextImage(m_Device);
    return (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR); // Continue only if we got a valid image
}

void vk_test::Application::waitForFrameCompletion() const {
    // Wait until GPU has finished processing the frame that was using these resources previously (numFramesInFlight frames ago)
    const VkSemaphoreWaitInfo wait_info = {
        .sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores    = &m_FrameTimelineSemaphore,
        .pValues        = &m_FrameData[m_FrameRingCurrent].frame_number,
    };
    vkWaitSemaphores(m_Device, &wait_info, std::numeric_limits<uint64_t>::max());
}

void vk_test::Application::freeResourcesQueue() {
    for (auto& func : m_ResourceFreeQueue[m_FrameRingCurrent]) {
        func(); // Free resources in queue
    }
    m_ResourceFreeQueue[m_FrameRingCurrent].clear();
}

void vk_test::Application::prepareFrameToSignal(int32_t num_frames_in_flight) {
    m_FrameData[m_FrameRingCurrent].frame_number += num_frames_in_flight;
}

//-----------------------------------------------------------------------
// Begin the command buffer recording for the frame
// It resets the command pool to reuse the command buffer for recording new rendering commands for the current frame.
// and it returns the command buffer for the frame.
VkCommandBuffer vk_test::Application::beginCommandRecording() {
    // Get the frame data for the current frame in the ring buffer
    FrameData& frame = m_FrameData[m_FrameRingCurrent];

    // Reset the command pool to reuse the command buffer for recording new rendering commands for the current frame.
    vkResetCommandPool(m_Device, frame.command_pool, 0);
    VkCommandBuffer command = frame.command_buffer;

    // Begin the command buffer recording for the frame
    const VkCommandBufferBeginInfo begin_info{ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                                               .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
    vkBeginCommandBuffer(command, &begin_info);

    return command;
}

void vk_test::Application::drawFrame(VkCommandBuffer command) {
    // Reset the extra semaphores and command buffers
    m_WaitSemaphores.clear();
    m_SignalSemaphores.clear();
    m_CommandBuffers.clear();

    // Call UI rendering for each element
    for (std::shared_ptr<IAppElement>& e : m_Elements) {
        e->onUIRender();
    }

    // Call onPreRender for each element with the command buffer of the frame
    for (std::shared_ptr<IAppElement>& e : m_Elements) {
        e->onPreRender();
    }

    // Call onRender for each element with the command buffer of the frame
    for (std::shared_ptr<IAppElement>& e : m_Elements) {
        e->onRender(command);
    }
}

void vk_test::Application::renderToSwapchain(VkCommandBuffer command) {
    // Start rendering to the swapchain
    beginDynamicRenderingToSwapchain(command);
    {
        //nvvk::DebugUtil::ScopedCmdLabel scopedCmdLabel(command, "ImGui");
        //// The ImGui draw commands are recorded to the command buffer, which includes the display of our GBuffer image
        //ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command);
    }
    endDynamicRenderingToSwapchain(command);
}

//-----------------------------------------------------------------------
// We are using dynamic rendering, which is a more flexible way to render to the swapchain image.
//
void vk_test::Application::beginDynamicRenderingToSwapchain(VkCommandBuffer command) const {
    // Image to render to
    const VkRenderingAttachmentInfo color_attachment{
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView   = m_Swapchain.getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,  // Clear the image (see clearValue)
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE, // Store the image (keep the image)
        .clearValue  = { { { 0.0F, 0.0F, 0.0F, 1.0F } } },
    };

    // Details of the dynamic rendering
    const VkRenderingInfo rendering_info{
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea           = { { 0, 0 }, m_WindowExtent },
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &color_attachment,
    };

    // Transition the swapchain image to the color attachment layout, needed when using dynamic rendering
    cmdImageMemoryBarrier(command, { m_Swapchain.getImage(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

    vkCmdBeginRendering(command, &rendering_info);
}

//-----------------------------------------------------------------------
// End of dynamic rendering.
// The image is transitioned back to the present layout, and the rendering is ended.
//
void vk_test::Application::endDynamicRenderingToSwapchain(VkCommandBuffer command) {
    vkCmdEndRendering(command);

    // Transition the swapchain image back to the present layout
    cmdImageMemoryBarrier(command, { m_Swapchain.getImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR });
}

void vk_test::Application::addSwapchainSemaphores() {
    // Prepare to submit the current frame for rendering
    // First add the swapchain semaphore to wait for the image to be available.
    m_WaitSemaphores.push_back({
        .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = m_Swapchain.getImageAvailableSemaphore(),
        .stageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    });
    m_SignalSemaphores.push_back({
        .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = m_Swapchain.getRenderFinishedSemaphore(),
        .stageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // Ensure everything is done before presenting
    });
}

void vk_test::Application::endFrame(VkCommandBuffer command, uint32_t frame_in_flights) {
    // Ends recording of commands for the frame
    vkEndCommandBuffer(command);

    // Get the frame data for the current frame in the ring buffer
    FrameData& frame = m_FrameData[m_FrameRingCurrent];

    // Add timeline semaphore to signal when GPU completes this frame
    // The color attachment output stage is used since that's when the frame is fully rendered
    m_SignalSemaphores.push_back({
        .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = m_FrameTimelineSemaphore,
        .value     = frame.frame_number,
        .stageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // Wait that everything is completed
    });

    // Adding the command buffer of the frame to the list of command buffers to submit
    // Note: extra command buffers could have been added to the list from other parts of the application (elements)
    m_CommandBuffers.push_back({ .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = command });

    // Populate the submit info to synchronize rendering and send the command buffer
    const VkSubmitInfo2 submitInfo{
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount   = uint32_t(m_WaitSemaphores.size()),   //
        .pWaitSemaphoreInfos      = m_WaitSemaphores.data(),             // Wait for the image to be available
        .commandBufferInfoCount   = uint32_t(m_CommandBuffers.size()),   //
        .pCommandBufferInfos      = m_CommandBuffers.data(),             // Command buffer to submit
        .signalSemaphoreInfoCount = uint32_t(m_SignalSemaphores.size()), //
        .pSignalSemaphoreInfos    = m_SignalSemaphores.data(),           // Signal when rendering is finished
    };

    // Submit the command buffer to the GPU and signal when it's done
    vkQueueSubmit2(m_Queues[0].queue, 1, &submitInfo, nullptr);
}

void vk_test::Application::presentFrame() {
    // Present the image
    m_Swapchain.presentFrame(m_Queues[0].queue);
}

void vk_test::Application::advanceFrame(uint32_t frame_in_flights) {
    // Move to the next frame
    m_FrameRingCurrent = (m_FrameRingCurrent + 1) % frame_in_flights;
}
