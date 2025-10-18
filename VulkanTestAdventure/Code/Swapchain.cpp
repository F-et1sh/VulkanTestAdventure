/*
 * Copyright (c) 2023-2025, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2023-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
* Modified by Farrakh
* 2025
*/

#include "pch.h"
#include "Swapchain.hpp"

#include "barriers.hpp"

VkResult vk_test::Swapchain::init(const InitInfo& info) {
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
        VK_TEST_SAY("ERROR : Selected queue family " << info.queue.family_index << " cannot present on surface " << info.surface << ". Swapchain creation failed");
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    return VK_SUCCESS;
}

VkResult vk_test::Swapchain::initResources(VkExtent2D& out_window_size, bool vsync) {
    // Query the physical device's capabilities for the given surface.
    const VkPhysicalDeviceSurfaceInfo2KHR surface_info2{
        .sType   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
        .surface = m_Surface
    };
    VkSurfaceCapabilities2KHR capabilities2{ .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };
    vkGetPhysicalDeviceSurfaceCapabilities2KHR(m_PhysicalDevice, &surface_info2, &capabilities2);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormats2KHR(m_PhysicalDevice, &surface_info2, &format_count, nullptr);
    std::vector<VkSurfaceFormat2KHR> formats(format_count, { .sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR });
    vkGetPhysicalDeviceSurfaceFormats2KHR(m_PhysicalDevice, &surface_info2, &format_count, formats.data());

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &present_mode_count, nullptr);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &present_mode_count, present_modes.data());

    // Choose the best available surface format and present mode
    const VkSurfaceFormat2KHR surface_format2 = selectSwapSurfaceFormat(formats);
    const VkPresentModeKHR    present_mode    = selectSwapPresentMode(present_modes, vsync);
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
    uint32_t image_count = 0;
    vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &image_count, nullptr);
    // On llvmpipe for instance, we can get more images than the minimum requested.
    // We still need to get a handle for each image in the swapchain
    // (because vkAcquireNextImageKHR can return an index to each image),
    // so adjust m_maxFramesInFlight.
    assert((m_maxFramesInFlight <= imageCount) && "Wrong swapchain setup");
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
