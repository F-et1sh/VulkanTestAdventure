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
#include "spirv.hpp"

#include "file_operations.hpp"
#include "hash_operations.hpp"

std::size_t vk_test::hashSpirv(const uint32_t* spirv_data, size_t spirv_size) {
    std::size_t seed = 0;
    for (size_t i = 0; i < spirv_size / sizeof(uint32_t); ++i) {
        vk_test::hashCombine(seed, spirv_data[i]);
    }
    return seed;
}

std::filesystem::path vk_test::dumpSpirvName(const std::filesystem::path& filename, const uint32_t* spirv_data, size_t spirv_size) {
    return vk_test::getExecutablePath().parent_path() / (filename.filename().replace_extension(std::to_string(hashSpirv(spirv_data, spirv_size)) + ".spv"));
}

void vk_test::dumpSpirv(const std::filesystem::path& filename, const uint32_t* spirv_data, size_t spirv_size) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        VK_TEST_SAY("Failed to open file for writing : " << vk_test::utf8FromPath(filename).c_str());
        return;
    }

    file.write(reinterpret_cast<const char*>(spirv_data), spirv_size);
    if (!file) {
        VK_TEST_SAY("Failed to write SPIR-V data to file : " << vk_test::utf8FromPath(filename).c_str());
    }
}

void vk_test::dumpSpirvWithHashedName(const std::filesystem::path& source_file, const uint32_t* spirv_data, size_t spirv_size) {
    dumpSpirv(dumpSpirvName(source_file, spirv_data, spirv_size), spirv_data, spirv_size);
}
