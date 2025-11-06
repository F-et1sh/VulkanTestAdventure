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

#include <filesystem>

#include <glm/glm.hpp>

#include "io_gltf.h" // Contains definitions for GLTF GltfMesh, BufferView, TriangleMesh and more

#include "resources.hpp"
#include "staging.hpp"
#include "bounding_box.hpp"

#include "primitives.hpp"
#include "tiny_gltf.h"

namespace vk_test {

    // Simple scene resource that holds meshes, instances, and materials
    struct GltfSceneResource {
        std::vector<shaderio::GltfMesh>              meshes{};    // All meshes in the scene
        std::vector<shaderio::GltfInstance>          instances{}; // All instances in the scene
        std::vector<shaderio::GltfMetallicRoughness> materials{}; // All materials in the scene
        shaderio::GltfSceneInfo                      sceneInfo{}; // Scene information (camera matrices, meshes, instances, materials, etc.)

        // GPU buffers for the scene data
        std::vector<vk_test::Buffer> b_gltf_datas{}; // Buffers containing the GLTF binary data for each loaded scene
        vk_test::Buffer              b_meshes;     // Buffer containing all GltfMesh data
        vk_test::Buffer              b_instances;  // Buffer containing all GltfInstance data
        vk_test::Buffer              b_materials;  // Buffer containing all GltfMetallicRoughness data
        vk_test::Buffer              b_scene_info; // Buffer containing GltfSceneInfo

        // Mapping from mesh index to buffer index in bGltfDatas
        std::vector<uint32_t> mesh_to_buffer_index{}; // meshToBufferIndex[meshIndex] = bufferIndex

        ~GltfSceneResource() = default;
    };

    // This is a utility function to load a GLTF file and return the model data.
    tinygltf::Model loadGltfResources(const std::filesystem::path& filename);

    // This is a utility function to import the GLTF data into the scene resource.
    void importGltfData(GltfSceneResource&        scene_resource,
                        const tinygltf::Model&    model,
                        vk_test::StagingUploader& staging_uploader,
                        bool                      import_instance = false);

    // This is a utility function to create the scene info buffer.
    void createGltfSceneInfoBuffer(GltfSceneResource& scene_resource, vk_test::StagingUploader& staging_uploader);

    // This is a utility function to convert a primitive mesh to a GltfMeshResource.
    void primitiveMeshToResource(GltfSceneResource& scene_resource, vk_test::StagingUploader& staging_uploader, const vk_test::PrimitiveMesh& prim_mesh);

} // namespace vk_test
