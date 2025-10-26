#include "pch.h"
#include "Application.hpp"

vk_test::Application::~Application() {
    vkDeviceWaitIdle(m_Device);

    // Clean pending
    //resetFreeQueue(0);

    //m_Swapchain.Release();

    // Frame info
    for (size_t i = 0; i < m_FrameData.size(); i++) {
        vkFreeCommandBuffers(m_Device, m_FrameData[i].command_pool, 1, &m_FrameData[i].command_buffer);
        vkDestroyCommandPool(m_Device, m_FrameData[i].command_pool, nullptr);
    }
    vkDestroySemaphore(m_Device, m_FrameTimelineSemaphore, nullptr);

    vkDestroyCommandPool(m_Device, m_TransientCmdPool, nullptr);
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
    //testAndSetWindowSizeAndPos({ info.windowSize.x, info.windowSize.y });

    // Used for creating single-time command buffers
    //createTransientCommandPool();

    // Create a descriptor pool for creating descriptor set in the application
    //createDescriptorPool();

    // Create the swapchain
    //if (!m_headless) {
    //    nvvk::Swapchain::InitInfo swapChainInit{
    //        .physicalDevice        = m_physicalDevice,
    //        .device                = m_device,
    //        .queue                 = m_queues[0],
    //        .surface               = m_surface,
    //        .cmdPool               = m_transientCmdPool,
    //        .preferredVsyncOffMode = info.preferredVsyncOffMode,
    //        .preferredVsyncOnMode  = info.preferredVsyncOnMode,
    //    };
    //
    //    NVVK_CHECK(m_swapchain.init(swapChainInit));
    //    NVVK_CHECK(m_swapchain.initResources(m_windowSize, m_vsyncWanted)); // Update the window size to the actual size of the surface
    //
    //    // Create what is needed to submit the scene for each frame in-flight
    //    createFrameSubmission(m_swapchain.getMaxFramesInFlight());
    //}
    //else {
    //    // In headless mode, there's only 2 pipeline stages (CPU and GPU, no display),
    //    // so we double instead of triple-buffer.
    //    createFrameSubmission(2);
    //}

    // Set up the resource free queue
    //resetFreeQueue(getFrameCycleSize());
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
