#include "pch.h"
#include "Renderer.h"

void VKTest::Renderer::CreateRenderPass() {
    auto swapchain_format = m_SwapchainManager.getFormat();

    vk::AttachmentDescription color_attachment{
        vk::AttachmentDescriptionFlags{},   // Flags
        swapchain_format,                   // Swapchain Image Format
        vk::SampleCountFlagBits::e1,        // Samples
        vk::AttachmentLoadOp::eClear,       // LoadOp
        vk::AttachmentStoreOp::eStore,      // StoreOp
        vk::AttachmentLoadOp::eDontCare,    // Stencil LoadOp
        vk::AttachmentStoreOp::eDontCare,   // Stencil StoreOp
        vk::ImageLayout::eUndefined,        // Initial Layout
        vk::ImageLayout::ePresentSrcKHR     // Final Layout
    };

    vk::AttachmentReference color_attachment_ref{
        0,                                          // Attachment
        vk::ImageLayout::eColorAttachmentOptimal    // Layout
    };

    vk::SubpassDescription subpass{
        vk::SubpassDescriptionFlags{},      // Flags
        vk::PipelineBindPoint::eGraphics,   // Pipeline Bind Point
        0,                                  // Input Attachment Count
        nullptr,                            // Input Attachments
        1,                                  // Color Attachment Count
        &color_attachment_ref               // Color Attachments
    };

    vk::SubpassDependency dependency{
        VK_SUBPASS_EXTERNAL,                                // Source Subpass
        0,                                                  // Destination Subpass
        vk::PipelineStageFlagBits::eColorAttachmentOutput,  // Source Stage Mask
        vk::PipelineStageFlagBits::eColorAttachmentOutput,  // Destination Stage Mask
        {},                                                 // Source Access Mask
        vk::AccessFlagBits::eColorAttachmentWrite           // Destination Access Mask
    };

    vk::RenderPassCreateInfo render_pass_info{
        vk::RenderPassCreateFlags{},    // Flags
        1,                              // Color Attachment Count
        &color_attachment,              // Color Attachments
        1,                              // Subpass Count
        &subpass,                       // Subpasses
        1,                              // Dependency Count
        &dependency                     // Dependencies
    };

    auto& device = m_DeviceManager.getDevice();
    m_RenderPass = vk::raii::RenderPass{ device, render_pass_info };
}

void VKTest::Renderer::CreateColorResources() {
    vk::Format color_format = this->m_SwapchainManager.getFormat();
    vk::Extent2D extent = this->m_SwapchainManager.getExtent();
    constexpr static uint32_t mip_levels = 1;

    m_DeviceManager.createImage(extent.width, extent.height, mip_levels, m_MSAA_Samples, color_format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, m_ColorImage, m_ColorImageMemory);
    auto image_view = m_DeviceManager.createImageView(m_ColorImage, color_format, vk::ImageAspectFlagBits::eColor, mip_levels);

    m_ColorImageView   = std::move(image_view);
}

void VKTest::Renderer::CreateDepthResources() {
    vk::Format depth_format = m_DeviceManager.findDepthFormat();
    vk::Extent2D extent = this->m_SwapchainManager.getExtent();
    constexpr static uint32_t mip_levels = 1;

    m_DeviceManager.createImage(extent.width, extent.height, mip_levels, m_MSAA_Samples, depth_format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, m_DepthImage, m_DepthImageMemory);
    auto image_view = m_DeviceManager.createImageView(m_DepthImage, depth_format, vk::ImageAspectFlagBits::eDepth, mip_levels);

    m_DepthImageView = std::move(image_view);
}
