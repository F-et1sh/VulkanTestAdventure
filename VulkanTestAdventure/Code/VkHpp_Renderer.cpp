#include "pch.h"
#include "VkHpp_Renderer.h"

void VKHppTest::Renderer::DrawFrame() {
    auto& device                     = m_DeviceManager.getDevice();
    auto& fences                     = m_GPUResourceManager.getInFlightFences();
    auto& image_available_semaphores = m_GPUResourceManager.getImageAvailableSemaphores();
    auto& render_finished_semaphores = m_GPUResourceManager.getRenderFinishedSemaphores();
    auto& swapchain                  = m_SwapchainManager.getSwapchain();
    auto& command_buffers            = m_DeviceManager.getCommandBuffers();
    auto& graphics_queue             = m_DeviceManager.getGraphicsQueue();

    vk::Result wait_result = device.waitForFences(*fences[m_CurrentFrame], vk::True, std::numeric_limits<uint64_t>::max());
    device.resetFences(*fences[m_CurrentFrame]);

    auto [result, image_index] = swapchain.acquireNextImage(std::numeric_limits<uint64_t>::max(), image_available_semaphores[m_CurrentFrame], {});

    if (result == vk::Result::eErrorOutOfDateKHR) {
        // TODO : this->RecreateSwapchain();
        return;
    } else if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        RUNTIME_ERROR("Failed to acquire swap chain image");
    }

    command_buffers[m_CurrentFrame].reset();
    m_PipelineManager.recordCommandBuffer(command_buffers[m_CurrentFrame], image_index);

    vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo         submit_info{
        1,                                            // Wait Semaphore Count
        &*image_available_semaphores[m_CurrentFrame], // Wait Semaphores
        &wait_stages,                                 // Wait Stage Mask
        1,                                            // Command Buffer Count
        &*command_buffers[m_CurrentFrame],            // Command Buffers
        1,                                            // Signal Semaphore Count
        &*render_finished_semaphores[m_CurrentFrame]  // Signal Semaphores
    };

    graphics_queue.submit(submit_info, fences[m_CurrentFrame]);

    vk::PresentInfoKHR present_info{
        1,                                            // Wait Semaphore Count
        &*render_finished_semaphores[m_CurrentFrame], // Wait Semaphores
        1,                                            // Swapchain Count
        &*swapchain,                                  // Swapchains
        &image_index                                  // Image Indices
    };

    result = graphics_queue.presentKHR(present_info);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR /* TODO : || m_FramebufferResized*/) {
        // TODO : m_FramebufferResized = false;
        // TODO : RecreateSwapchain();
    } else if (result != vk::Result::eSuccess)
        RUNTIME_ERROR("Failed to present swap chain image");

    m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
