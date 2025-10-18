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

#pragma once

#include <cassert>
#include <vector>

#include "resources.hpp"

//--- Swapchain ------------------------------------------------------------------------------------------------------------
/*--
 * Swapchain: The swapchain is responsible for presenting rendered images to the screen.
 * It consists of multiple images (frames) that are cycled through for rendering and display.
 * The swapchain is created with a surface and optional vSync setting, with the
 * window size determined during its setup.
 * "Frames in flight" refers to the number of images being processed concurrently (e.g., double buffering = 2, triple buffering = 3).
 * vSync enabled (FIFO mode) uses double buffering, while disabling vSync  (MAILBOX mode) uses triple buffering.
 *
 * The "current frame" is the frame currently being processed.
 * The "next image index" points to the swapchain image that will be rendered next, which might differ from the current frame's index.
 * If the window is resized or certain conditions are met, the swapchain needs to be recreated (`needRebuild` flag).
-*/
namespace vk_test {
    class Swapchain {
    public:
        Swapchain() = default;
        ~Swapchain() { assert(m_Swapchain == VK_NULL_HANDLE && "Missing deinit()"); }

        void        requestRebuild() { m_NeedRebuild = true; }
        bool        needRebuilding() const { return m_NeedRebuild; }
        VkImage     getImage() const { return m_Images[m_FrameImageIndex].image; }
        VkImageView getImageView() const { return m_Images[m_FrameImageIndex].image_view; }
        VkFormat    getImageFormat() const { return m_ImageFormat; }
        uint32_t    getMaxFramesInFlight() const { return m_MaxFramesInFlight; }
        VkSemaphore getImageAvailableSemaphore() const {
            return m_FrameResources[m_FrameResourceIndex].image_available_semaphore;
        }
        VkSemaphore getRenderFinishedSemaphore() const { return m_FrameResources[m_FrameImageIndex].render_finished_semaphore; }

        struct InitInfo {
            VkPhysicalDevice physical_device{};
            VkDevice         device{};
            QueueInfo        queue{};
            VkSurfaceKHR     surface{};
            VkCommandPool    command_pool{};
            VkPresentModeKHR preferred_vsync_off_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            VkPresentModeKHR preferred_vsync_on_mode  = VK_PRESENT_MODE_FIFO_KHR;
        };

        // Initialize the swapchain with the provided context and surface, then we can create and re-create it
        VkResult init(const InitInfo& info);

        // Destroy internal resources and reset its initial state
        void deinit();

        /*--
         * Create the swapchain using the provided context, surface, and vSync option. The actual window size is returned.
         * Queries the GPU capabilities, selects the best surface format and present mode, and creates the swapchain accordingly.
        -*/
        VkResult initResources(VkExtent2D& out_window_size, bool vsync = true);

        /*--
         * Recreate the swapchain, typically after a window resize or when it becomes invalid.
         * This waits for all rendering to be finished before destroying the old swapchain and creating a new one.
        -*/
        VkResult reinitResources(VkExtent2D& out_window_size, bool v_sync = true);

        /*--
         * Destroy the swapchain and its associated resources.
         * This function is also called when the swapchain needs to be recreated.
        -*/
        void deinitResources();

        /*--
         * Prepares the command buffer for recording rendering commands.
         * This function handles synchronization with the previous frame and acquires the next image from the swapchain.
         * The command buffer is reset, ready for new rendering commands.
        -*/
        VkResult acquireNextImage(VkDevice device);

        /*--
         * Presents the rendered image to the screen.
         * The semaphore ensures that the image is presented only after rendering is complete.
         * Advances to the next frame in the cycle.
        -*/
        void presentFrame(VkQueue queue);

    private:
        // Represents an image within the swapchain that can be rendered to.
        struct Image {
            VkImage     image{};      // Image to render to
            VkImageView image_view{}; // Image view to access the image
        };
        /*--
         * Resources associated with each frame being processed.
         * Each frame has its own set of resources, mainly synchronization primitives
        -*/
        struct FrameResources {
            VkSemaphore image_available_semaphore{}; // Signals when the image is ready for rendering
            VkSemaphore render_finished_semaphore{}; // Signals when rendering is finished
        };

        // We choose the format that is the most common, and that is supported by* the physical device.
        VkSurfaceFormat2KHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormat2KHR>& available_formats) const;

        /*--
         * The present mode is chosen based on the vSync option
         * The `preferredVsyncOnMode` is used when vSync is enabled and the mode is supported.
         * The `preferredVsyncOffMode` is used when vSync is disabled and the mode is supported.
         * Otherwise, from most preferred to least:
         *   1. IMMEDIATE mode, when vSync is disabled (tearing allowed), since it's lowest-latency.
         *   2. MAILBOX mode, since it's the lowest-latency mode without tearing. Note that frame pacing is needed when vSync is on.
         *   3. FIFO mode, since all swapchains must support it.
        -*/
        VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes, bool v_sync = true) const;

    private:
        Swapchain(Swapchain&)                  = delete;  //
        Swapchain& operator=(const Swapchain&) = default; // Allow copy only for internal use in deinit() to restore pristine state

        VkPhysicalDevice m_PhysicalDevice{}; // The physical device (GPU)
        VkDevice         m_Device{};         // The logical device (interface to the physical device)
        QueueInfo        m_Queue{};          // The queue used to submit command buffers to the GPU
        VkSwapchainKHR   m_Swapchain{};      // The swapchain
        VkFormat         m_ImageFormat{};    // The format of the swapchain images
        VkSurfaceKHR     m_Surface{};        // The surface to present images to
        VkCommandPool    m_CommandPool{};    // The command pool for the swapchain

        std::vector<Image>          m_Images;                     // The swapchain images and their views
        std::vector<FrameResources> m_FrameResources;             // Synchronization primitives for each frame
        uint32_t                    m_FrameResourceIndex = 0;     // Current frame index, cycles through frame resources
        uint32_t                    m_FrameImageIndex    = 0;     // Index of the swapchain image we're currently rendering to
        bool                        m_NeedRebuild        = false; // Flag indicating if the swapchain needs to be rebuilt

        VkPresentModeKHR m_PreferredVsyncOffMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // used if available
        VkPresentModeKHR m_PreferredVsyncOnMode  = VK_PRESENT_MODE_FIFO_KHR;      // used if available

        // Triple buffering allows us to pipeline CPU and GPU work, which gives us
        // good throughput if their sum takes more than a frame.
        // But if we're using VK_PRESENT_MODE_FIFO_KHR without frame pacing and
        // workloads are < 1 frame, then work can be waiting for multiple frames for
        // the swapchain image to be available, increasing latency. For this reason,
        // it's good to use a frame pacer with the swapchain.
        uint32_t m_MaxFramesInFlight = 3;
    };
} // namespace vk_test
