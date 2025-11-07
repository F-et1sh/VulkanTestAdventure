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

#pragma once

#include "resources.hpp"
#include "staging.hpp"

namespace vk_test {

    inline static VkShaderModuleCreateInfo getShaderModuleCreateInfo(const std::span<const uint32_t>& spirv) {
        return VkShaderModuleCreateInfo{
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = spirv.size_bytes(),
            .pCode    = spirv.data(),
        };
    }

    Image loadAndCreateImage(VkCommandBuffer              cmd,
                             StagingUploader&             staging,
                             VkDevice                     device,
                             const std::filesystem::path& filename,
                             bool                         s_rgb = true);

} // namespace vk_test
