#include "pch.h"
#include "Swapchain.hpp"

VkResult Swapchain::Initialize(const Swapchain::InitInfo& info) {
    m_PhysicalDevice = info.physical_device;
    m_Device         = info.device;
    m_Queue          = info.queue;
    m_Surface        = info.surface;
    m_CommandPool    = info.command_pool;

    if (info.preferred_vsync_off_mode != VK_PRESENT_MODE_MAX_ENUM_KHR) {
        m_PreferredVsyncOffMode = info.preferred_vsync_off_mode;
    }
    if (info.preferred_vsync_on_mode != VK_PRESENT_MODE_MAX_ENUM_KHR) {
        m_PreferredVsyncOnMode = info.preferred_vsync_on_mode;
    }

    VkBool32 supports_present = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(info.physical_device, info.queue.family_index, info.surface, &supports_present);

    if (supports_present != VK_TRUE) {
        VK_TEST_SAY("ERROR : Selected queue family" << info.queue.family_index << "cannot present on surface " << info.surface << ". Swapchain creation failed");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    return VK_SUCCESS;
}

VkResult vk_test::Swapchain::InitializeResources(VkExtent2D& out_window_size, bool v_sync) {
    // Query the physical device's capabilities for the given surface.
    const VkPhysicalDeviceSurfaceInfo2KHR surface_info2{ .sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
                                                         .surface = m_Surface };
    VkSurfaceCapabilities2KHR             capabilities2{ .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };
    vkGetPhysicalDeviceSurfaceCapabilities2KHR(m_PhysicalDevice, &surface_info2, &capabilities2);

    uint32_t format_count{};
    vkGetPhysicalDeviceSurfaceFormats2KHR(m_PhysicalDevice, &surface_info2, &format_count, nullptr);
    std::vector<VkSurfaceFormat2KHR> formats(format_count, { .sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR });
    vkGetPhysicalDeviceSurfaceFormats2KHR(m_PhysicalDevice, &surface_info2, &format_count, formats.data());

    uint32_t present_mode_count{};
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &present_mode_count, nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &present_mode_count, present_modes.data());

    // Choose the best available surface format and present mode
    const VkSurfaceFormat2KHR surface_format2 = selectSwapSurfaceFormat(formats);
    const VkPresentModeKHR    present_mode    = selectSwapPresentMode(present_modes, v_sync);
    // Set the window size according to the surface's current extent
    out_window_size = capabilities2.surfaceCapabilities.currentExtent;
    // Set the number of images in flight, respecting the GPU's maxImageCount limit.
    // If maxImageCount is equal to 0, then there is no limit other than memory,
    // so don't change m_maxFramesInFlight.
    if (capabilities2.surfaceCapabilities.maxImageCount > 0) {
        m_MaxFramesInFlight = std::min(m_MaxFramesInFlight, capabilities2.surfaceCapabilities.maxImageCount);
    }
    // Store the chosen image format
    m_ImageFormat = surface_format2.surfaceFormat.format;

    // Create the swapchain itself
    const VkSwapchainCreateInfoKHR swapchain_create_info{
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = m_Surface,
        .minImageCount    = m_MaxFramesInFlight,
        .imageFormat      = surface_format2.surfaceFormat.format,
        .imageColorSpace  = surface_format2.surfaceFormat.colorSpace,
        .imageExtent      = capabilities2.surfaceCapabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform     = capabilities2.surfaceCapabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = present_mode,
        .clipped          = VK_TRUE,
    };
    vkCreateSwapchainKHR(m_Device, &swapchain_create_info, nullptr, &m_Swapchain);

    // Retrieve the swapchain images
    uint32_t image_count{};
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &image_count, nullptr);
    // On llvmpipe for instance, we can get more images than the minimum requested.
    // We still need to get a handle for each image in the swapchain
    // (because vkAcquireNextImageKHR can return an index to each image),
    // so adjust m_maxFramesInFlight.
    assert((m_MaxFramesInFlight <= image_count) && "Wrong swapchain setup");

    m_MaxFramesInFlight = image_count;
    std::vector<VkImage> swap_images(m_MaxFramesInFlight);
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &m_MaxFramesInFlight, swap_images.data());

    // Store the swapchain images and create views for them
    m_Images.resize(m_MaxFramesInFlight);
    VkImageViewCreateInfo image_view_create_info{
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = m_ImageFormat,
        .components       = { .r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1 },
    };
    for (uint32_t i = 0; i < m_MaxFramesInFlight; i++) {
        m_Images[i].image            = swap_images[i];
        image_view_create_info.image = m_Images[i].image;
        vkCreateImageView(m_Device, &image_view_create_info, nullptr, &m_Images[i].image_view);
    }

    // Initialize frame resources for each swapchain image
    m_FrameResources.resize(m_MaxFramesInFlight);
    for (size_t i = 0; i < m_MaxFramesInFlight; ++i) {
        /*--
         * The sync objects are used to synchronize the rendering with the presentation.
         * The image available semaphore is signaled when the image is available to render.
         * The render finished semaphore is signaled when the rendering is finished.
         * The in flight fence is signaled when the frame is in flight.
        -*/
        const VkSemaphoreCreateInfo semaphore_create_info{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        vkCreateSemaphore(m_Device, &semaphore_create_info, nullptr, &m_FrameResources[i].image_available_semaphore);
        vkCreateSemaphore(m_Device, &semaphore_create_info, nullptr, &m_FrameResources[i].render_finished_semaphore);
    }

    // Transition images to present layout
    {
        VkCommandBuffer cmd{};
        beginSingleTimeCommands(cmd, m_Device, m_CommandPool);
        for (uint32_t i = 0; i < m_MaxFramesInFlight; i++) {
            cmdImageMemoryBarrier(cmd, { m_Images[i].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR });
        }
        endSingleTimeCommands(cmd, m_Device, m_CommandPool, m_Queue.queue);
    }

    return VK_SUCCESS;
}

VkResult vk_test::Swapchain::ReinitializeResources(VkExtent2D& out_window_size, bool v_sync) {
    // Wait for all frames to finish rendering before recreating the swapchain
    vkQueueWaitIdle(m_Queue.queue);

    m_FrameResourceIndex = 0;
    m_NeedRebuild        = false;
    ReleaseResources();
    return InitializeResources(out_window_size, v_sync);
}

void vk_test::Swapchain::ReleaseResources() {
    vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
    for (auto& frame_res : m_FrameResources) {
        vkDestroySemaphore(m_Device, frame_res.image_available_semaphore, nullptr);
        vkDestroySemaphore(m_Device, frame_res.render_finished_semaphore, nullptr);
    }
    m_FrameResources.clear();
    for (auto& image : m_Images) {
        vkDestroyImageView(m_Device, image.image_view, nullptr);
    }
    m_Images.clear();
}

VkResult vk_test::Swapchain::acquireNextImage(VkDevice device) {
    assert((m_NeedRebuild == false) && "Swapbuffer need to call reinitResources()");

    // Get the frame resources for the current frame
    // We use m_currentFrame here because we want to ensure we don't overwrite resources
    // that are still in use by previous frames
    auto& frame = m_FrameResources[m_FrameResourceIndex];

    // Acquire the next image from the swapchain
    // This will signal frame.imageAvailableSemaphore when the image is ready
    // and store the index of the acquired image in m_nextImageIndex
    VkResult result = vkAcquireNextImageKHR(device, m_Swapchain, std::numeric_limits<uint64_t>::max(), frame.image_available_semaphore, VK_NULL_HANDLE, &m_FrameImageIndex);

    switch (result) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR: // Still valid for presentation
            return result;

        case VK_ERROR_OUT_OF_DATE_KHR: // The swapchain is no longer compatible with the surface and needs to be recreated
            m_NeedRebuild = true;
            return result;

        default:
            VK_TEST_SAY("Failed to acquire swapchain image : " << result);
            return result;
    }
}

void vk_test::Swapchain::presentFrame(VkQueue queue) {
    // Get the frame resources for the current image
    // We use m_nextImageIndex here because we want to signal the semaphore
    // associated with the image we just finished rendering
    auto& frame = m_FrameResources[m_FrameImageIndex];

    // Setup the presentation info, linking the swapchain and the image index
    const VkPresentInfoKHR presentInfo{
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,                                // Wait for rendering to finish
        .pWaitSemaphores    = &frame.render_finished_semaphore, // Synchronize presentation
        .swapchainCount     = 1,                                // Swapchain to present the image
        .pSwapchains        = &m_Swapchain,                     // Pointer to the swapchain
        .pImageIndices      = &m_FrameImageIndex,               // Index of the image to present
    };

    // Present the image and handle potential resizing issues
    const VkResult result = vkQueuePresentKHR(queue, &presentInfo);
    // If the swapchain is out of date (e.g., window resized), it needs to be rebuilt
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        m_NeedRebuild = true;
    }
    else {
        assert((result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) && "Couldn't present swapchain image");
    }

    // Advance to the next frame in the swapchain
    m_FrameResourceIndex = (m_FrameResourceIndex + 1) % m_MaxFramesInFlight;
}

void Swapchain::Release() {
    if (m_Device != nullptr) {
        ReleaseResources();
    }
    *this = {};
}

VkSurfaceFormat2KHR vk_test::Swapchain::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormat2KHR>& available_formats) {
    // If there's only one available format and it's undefined, return a default format.
    if (available_formats.size() == 1 && available_formats[0].surfaceFormat.format == VK_FORMAT_UNDEFINED) {
        VkSurfaceFormat2KHR result{ .sType         = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR,
                                    .surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } };
        return result;
    }

    const std::vector<VkSurfaceFormat2KHR> preferred_formats = {
        VkSurfaceFormat2KHR{ .sType         = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR,
                             .surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } },
        VkSurfaceFormat2KHR{ .sType         = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR,
                             .surfaceFormat = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR } }
    };

    // Check available formats against the preferred formats.
    for (const auto& preferred_format : preferred_formats) {
        for (const auto& available_format : available_formats) {
            if (available_format.surfaceFormat.format == preferred_format.surfaceFormat.format && available_format.surfaceFormat.colorSpace == preferred_format.surfaceFormat.colorSpace) {
                return available_format; // Return the first matching preferred format.
            }
        }
    }

    // If none of the preferred formats are available, return the first available format.
    return available_formats[0];
}

VkPresentModeKHR vk_test::Swapchain::selectSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes, bool v_sync) {
    bool mailbox_supported   = false;
    bool immediate_supported = false;

    for (VkPresentModeKHR mode : available_present_modes) {
        if (v_sync && (mode == m_PreferredVsyncOnMode)) {
            return mode;
        }
        if (!v_sync && (mode == m_PreferredVsyncOffMode)) {
            return mode;
        }

        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            mailbox_supported = true;
        }
        if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            immediate_supported = true;
        }
    }

    if (!v_sync && immediate_supported) {
        return VK_PRESENT_MODE_IMMEDIATE_KHR; // Best mode for low latency
    }

    if (mailbox_supported) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}
