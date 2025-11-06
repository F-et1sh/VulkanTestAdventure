/*
 * Copyright (c) 2024-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2024-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
* Modified by Farrakh
* 2025
*/

#include "pch.h"
#include "utils.hpp"

#include <staging.hpp>
#include <default_structs.hpp>
#include <timers.hpp>
#include <file_operations.hpp>

#include <stb_image.h>

namespace vk_test {

    vk_test::Image loadAndCreateImage(VkCommandBuffer cmd, vk_test::StagingUploader& staging, VkDevice device, const std::filesystem::path& filename, bool s_rgb) {
        // Load the image from disk
        int            w;
        int            h;
        int            comp;
        int            req_comp{ 4 };
        std::string    filename_utf8 = vk_test::utf8FromPath(filename);
        const stbi_uc* data          = stbi_load(filename_utf8.c_str(), &w, &h, &comp, req_comp);
        assert((data != nullptr) && "Could not load texture image!");

        // Define how to create the image
        VkImageCreateInfo image_info = DEFAULT_VkImageCreateInfo;
        image_info.format            = s_rgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        image_info.usage             = VK_IMAGE_USAGE_SAMPLED_BIT;
        image_info.extent            = { uint32_t(w), uint32_t(h), 1 };

        vk_test::ResourceAllocator* allocator = staging.getResourceAllocator();

        // Use the VMA allocator to create the image
        const std::span data_span(data, w * h * req_comp);
        vk_test::Image  texture;
        allocator->createImage(texture, image_info, DEFAULT_VkImageViewCreateInfo);
        staging.appendImage(texture, data_span, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        return texture;
    }

} // namespace vk_test
