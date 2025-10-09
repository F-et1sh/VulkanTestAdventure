#include "pch.h"
#include "VkHpp_RenderPassManager.h"

RenderPassManager::RenderPassManager(DeviceManager* device_manager, SwapchainManager* swapchain_manager) : p_DeviceManager{ device_manager }, p_SwapchainManager{ swapchain_manager } {}

void VKHppTest::RenderPassManager::CreateRenderPass() {
    auto swapchain_format = p_SwapchainManager->getFormat();
    auto depth_format     = p_DeviceManager->findDepthFormat();

    vk::AttachmentDescription color_attachment{
        vk::AttachmentDescriptionFlags{}, // Flags
        swapchain_format,                 // Swapchain Image Format
        m_MSAA_Samples,                   // Samples
        vk::AttachmentLoadOp::eClear,     // LoadOp
        vk::AttachmentStoreOp::eStore,    // StoreOp
        vk::AttachmentLoadOp::eDontCare,  // Stencil LoadOp
        vk::AttachmentStoreOp::eDontCare, // Stencil StoreOp
        vk::ImageLayout::eUndefined,      // Initial Layout
        vk::ImageLayout::ePresentSrcKHR   // Final Layout
    };

    vk::AttachmentDescription depth_attachment{
        vk::AttachmentDescriptionFlags{},               // Flags
        depth_format,                                   // Depth Image Format
        m_MSAA_Samples,                                 // Samples
        vk::AttachmentLoadOp::eClear,                   // LoadOp
        vk::AttachmentStoreOp::eStore,                  // StoreOp
        vk::AttachmentLoadOp::eDontCare,                // Stencil LoadOp
        vk::AttachmentStoreOp::eDontCare,               // Stencil StoreOp
        vk::ImageLayout::eUndefined,                    // Initial Layout
        vk::ImageLayout::eDepthStencilAttachmentOptimal // Final Layout
    };

    vk::AttachmentReference color_attachment_ref{
        0,                                       // Attachment
        vk::ImageLayout::eColorAttachmentOptimal // Layout
    };

    vk::AttachmentReference depth_attachment_ref{
        1,                                              // Attachment
        vk::ImageLayout::eDepthStencilAttachmentOptimal // Layout
    };

    vk::SubpassDescription subpass{
        vk::SubpassDescriptionFlags{},    // Flags
        vk::PipelineBindPoint::eGraphics, // Pipeline Bind Point
        0,                                // Input Attachment Count
        nullptr,                          // Input Attachments
        1,                                // Color Attachment Count
        &color_attachment_ref,            // Color Attachments
        &depth_attachment_ref             // Depth Attachments
    };

    vk::SubpassDependency dependency{
        VK_SUBPASS_EXTERNAL,                               // Source Subpass
        0,                                                 // Destination Subpass
        vk::PipelineStageFlagBits::eColorAttachmentOutput, // Source Stage Mask
        vk::PipelineStageFlagBits::eColorAttachmentOutput, // Destination Stage Mask
        {},                                                // Source Access Mask
        vk::AccessFlagBits::eColorAttachmentWrite          // Destination Access Mask
    };

    std::array<vk::AttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

    vk::RenderPassCreateInfo render_pass_info{
        vk::RenderPassCreateFlags{},               // Flags
        static_cast<uint32_t>(attachments.size()), // Attachment Count
        attachments.data(),                        // Attachments
        1,                                         // Subpass Count
        &subpass,                                  // Subpasses
        1,                                         // Dependency Count
        &dependency                                // Dependencies
    };

    auto& device = p_DeviceManager->getDevice();
    m_RenderPass = vk::raii::RenderPass{ device, render_pass_info };
}
