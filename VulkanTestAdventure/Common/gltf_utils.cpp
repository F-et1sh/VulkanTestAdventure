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
#include "gltf_utils.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace vk_test {
    // This is a utility function to convert a primitive mesh to a GltfMeshResource.
    void primitiveMeshToResource(GltfSceneResource&   scene_resource,
                                 StagingUploader&     staging_uploader,
                                 const PrimitiveMesh& prim_mesh) {

        ResourceAllocator* allocator = staging_uploader.getResourceAllocator();

        // Calculate buffer sizes
        size_t vertices_size  = std::span(prim_mesh.vertices).size_bytes();
        size_t triangles_size = std::span(prim_mesh.triangles).size_bytes();

        // Create buffer for the geometry data (vertices + triangles)
        Buffer gltf_data;
        allocator->createBuffer(gltf_data, vertices_size + triangles_size, VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
        uint32_t buffer_index = static_cast<uint32_t>(scene_resource.b_gltf_datas.size());
        scene_resource.b_gltf_datas.push_back(gltf_data);

        // Upload vertices first (at offset 0)
        staging_uploader.appendBuffer(gltf_data, 0, std::span(prim_mesh.vertices));

        // Upload triangles after vertices
        staging_uploader.appendBuffer(gltf_data, vertices_size, std::span(prim_mesh.triangles));

        // Set up the TriangleMesh structure with proper BufferView offsets
        shaderio::GltfMesh mesh;
        mesh.tri_mesh.positions = { .offset      = 0,
                                    .count       = static_cast<uint32_t>(prim_mesh.vertices.size()),
                                    .byte_stride = sizeof(PrimitiveVertex) };

        mesh.tri_mesh.normals = { .offset      = offsetof(PrimitiveVertex, nrm),
                                  .count       = static_cast<uint32_t>(prim_mesh.vertices.size()),
                                  .byte_stride = sizeof(PrimitiveVertex) };

        mesh.tri_mesh.tex_coords = { .offset      = offsetof(PrimitiveVertex, tex),
                                     .count       = static_cast<uint32_t>(prim_mesh.vertices.size()),
                                     .byte_stride = sizeof(PrimitiveVertex) };

        mesh.tri_mesh.indices = { .offset      = static_cast<uint32_t>(vertices_size),
                                  .count       = static_cast<uint32_t>(prim_mesh.triangles.size() * 3), // 3 indices per triangle
                                  .byte_stride = sizeof(uint32_t) };
        mesh.index_type       = VK_INDEX_TYPE_UINT32; // Assuming uint32_t indices

        // Set the buffer address and index type
        mesh.gltf_buffer = (uint8_t*) gltf_data.address;
        mesh.index_type  = VK_INDEX_TYPE_UINT32; // Assuming uint32_t indices
        scene_resource.meshes.push_back(mesh);

        // Update the mapping from mesh index to buffer index
        scene_resource.mesh_to_buffer_index.push_back(buffer_index);
    }

    tinygltf::Model loadGltfResources(const std::filesystem::path& filename) {
        tinygltf::TinyGLTF tiny_loader;
        tinygltf::Model    model;
        std::string        err;
        std::string        warn;
        if (filename.extension() == ".gltf") {
            if (!tiny_loader.LoadASCIIFromFile(&model, &err, &warn, filename.string())) {
                VK_TEST_SAY("Error loading glTF file : " << err.c_str());
                assert(0 && "No fallback");
                return {};
            }
        }
        else if (filename.extension() == ".glb") {
            if (!tiny_loader.LoadBinaryFromFile(&model, &err, &warn, filename.string())) {
                VK_TEST_SAY("Error loading glTF file : " << err.c_str());
                assert(0 && "No fallback");
                return {};
            }
        }
        else {
            VK_TEST_SAY("Unsupported file format : " << filename.extension().string().c_str());
            assert(0 && "No fallback");
            return {};
        }
        return model;
    }

    // This is a utility function to import the GLTF data into the scene resource.
    // It is a very simple function that just imports the GLTF data into the scene resource.
    // It has strong limitations, like the mesh must have only one primitive, and the primitive must be a triangle primitive.
    // But it allow to import the GLTF data into the scene resource with a single function call, and call it again to import another scene.
    void importGltfData(GltfSceneResource&     scene_resource,
                        const tinygltf::Model& model,
                        StagingUploader&       staging_uploader,
                        bool                   import_instance /*= false*/) {
        const uint32_t mesh_offset = uint32_t(scene_resource.meshes.size());

        // Lambda for element byte size calculation
        auto get_element_byte_size = [](int type) -> uint32_t {
            return type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? 2U : type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ? 4U
                                                                     : type == TINYGLTF_COMPONENT_TYPE_FLOAT          ? 4U
                                                                                                                      : 0U;
        };

        // Lambda for type size calculation
        auto get_type_size = [](int type) -> uint32_t {
            return type == TINYGLTF_TYPE_VEC2 ? 2U : type == TINYGLTF_TYPE_VEC3 ? 3U
                                                 : type == TINYGLTF_TYPE_VEC4   ? 4U
                                                 : type == TINYGLTF_TYPE_MAT2   ? 4U * 2U
                                                 : type == TINYGLTF_TYPE_MAT3   ? 4U * 3U
                                                 : type == TINYGLTF_TYPE_MAT4   ? 4U * 4U
                                                                                : 0U;
        };

        // Lambda for extracting attributes, like positions, normals, colors, etc.
        auto extract_attribute = [&](const std::string& name, shaderio::BufferView& attr, const tinygltf::Primitive& primitive) {
            if (!primitive.attributes.contains(name)) {
                attr.offset = -1;
                return;
            }
            const tinygltf::Accessor&   acc = model.accessors[primitive.attributes.at(name)];
            const tinygltf::BufferView& bv  = model.bufferViews[acc.bufferView];
            assert((acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) && "Should be floats");
            attr = {
                .offset      = uint32_t(bv.byteOffset + acc.byteOffset),
                .count       = uint32_t(acc.count),
                .byte_stride = (bv.byteStride ? uint32_t(bv.byteStride) : get_type_size(acc.type) * get_element_byte_size(acc.componentType)),
            };
        };

        // Upload the scene resource to the GPU
        Buffer   b_gltf_data;
        uint32_t buffer_index{};
        {
            ResourceAllocator* allocator = staging_uploader.getResourceAllocator();

            // The GLTF buffer is used to store the geometry data (indices, positions, normals, etc.)
            // The flags are set to allow the buffer to be used as a vertex buffer, index buffer, storage buffer, and for acceleration structure build input read-only.
            allocator->createBuffer(b_gltf_data, std::span<const unsigned char>(model.buffers[0].data).size_bytes(),
                                    VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR); // #RT
            staging_uploader.appendBuffer(b_gltf_data, 0, std::span<const unsigned char>(model.buffers[0].data));

            buffer_index = static_cast<uint32_t>(scene_resource.b_gltf_datas.size());
            scene_resource.b_gltf_datas.push_back(b_gltf_data);
        }

        for (size_t mesh_idx = 0; mesh_idx < model.meshes.size(); ++mesh_idx) {
            shaderio::GltfMesh mesh{};

            const tinygltf::Mesh&      tiny_mesh = model.meshes[mesh_idx];
            const tinygltf::Primitive& primitive = tiny_mesh.primitives.front();
            assert((tiny_mesh.primitives.size() == 1 && primitive.mode == TINYGLTF_MODE_TRIANGLES) && "Must have one triangle primitive");

            // Extract indices
            const auto& accessor    = model.accessors[primitive.indices];
            const auto& buffer_view = model.bufferViews[accessor.bufferView];
            assert((accessor.count % 3 == 0) && "Should be a multiple of 3");
            mesh.tri_mesh.indices = {
                .offset      = uint32_t(buffer_view.byteOffset + accessor.byteOffset),
                .count       = uint32_t(accessor.count),
                .byte_stride = uint32_t((buffer_view.byteStride != 0U) ? buffer_view.byteStride : get_element_byte_size(accessor.componentType)),
            };
            mesh.index_type = accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;

            // Set the buffer address
            mesh.gltf_buffer = (uint8_t*) b_gltf_data.address;

            // Extract attributes
            extract_attribute("POSITION", mesh.tri_mesh.positions, primitive);
            extract_attribute("NORMAL", mesh.tri_mesh.normals, primitive);
            extract_attribute("COLOR_0", mesh.tri_mesh.color_vert, primitive);
            extract_attribute("TEXCOORD_0", mesh.tri_mesh.tex_coords, primitive);
            extract_attribute("TANGENT", mesh.tri_mesh.tangents, primitive);

            scene_resource.meshes.emplace_back(mesh);

            // Update the mapping from mesh index to buffer index
            scene_resource.mesh_to_buffer_index.push_back(buffer_index);
        }

        if (import_instance) {
            // Extract instances with proper hierarchical transformation handling
            std::function<void(const tinygltf::Node&, const glm::mat4&)> process_node = [&](const tinygltf::Node& node,
                                                                                            const glm::mat4&      parent_transform) {
                glm::mat4 node_transform = parent_transform;

                // Apply node transform
                if (!node.matrix.empty()) {
                    // Use matrix if available
                    glm::mat4 matrix = glm::make_mat4(node.matrix.data());
                    node_transform   = parent_transform * matrix;
                }
                else {
                    // Apply TRS if matrix is not available
                    if (!node.translation.empty()) {
                        glm::vec3 translation = glm::make_vec3(node.translation.data());
                        node_transform        = glm::translate(node_transform, translation);
                    }
                    if (!node.rotation.empty()) {
                        glm::quat rotation = glm::make_quat(node.rotation.data());
                        node_transform     = node_transform * glm::mat4_cast(rotation);
                    }
                    if (!node.scale.empty()) {
                        glm::vec3 scale = glm::make_vec3(node.scale.data());
                        node_transform  = glm::scale(node_transform, scale);
                    }
                }

                // Create instance for this node if it has a mesh
                if (node.mesh != -1) {
                    const tinygltf::Mesh&      tiny_mesh = model.meshes[node.mesh];
                    const tinygltf::Primitive& primitive = tiny_mesh.primitives.front();
                    assert((tiny_mesh.primitives.size() == 1 && primitive.mode == TINYGLTF_MODE_TRIANGLES) && "Must have one triangle primitive");
                    shaderio::GltfInstance instance{};
                    instance.mesh_index = node.mesh + mesh_offset;
                    instance.transform  = node_transform;
                    scene_resource.instances.push_back(instance);
                }

                // Process children
                for (int child_idx : node.children) {
                    if (child_idx >= 0 && child_idx < static_cast<int>(model.nodes.size())) {
                        process_node(model.nodes[child_idx], node_transform);
                    }
                }
            };

            // Process all root nodes (nodes with no parent)
            for (size_t node_idx = 0; node_idx < model.nodes.size(); ++node_idx) {
                const tinygltf::Node& node = model.nodes[node_idx];
                // Check if this is a root node (not referenced as a child)
                bool is_root_node = true;
                for (const auto& other_node : model.nodes) {
                    for (int child_idx : other_node.children) {
                        if (child_idx == static_cast<int>(node_idx)) {
                            is_root_node = false;
                            break;
                        }
                    }
                    if (!is_root_node) {
                        break;
                    }
                }

                if (is_root_node) {
                    process_node(node, glm::mat4(1.0F)); // Start with identity matrix
                }
            }
        }
    }

    // This function creates the scene info buffer
    // It is consolidating all the mesh information into a single buffer, the same for the instances and materials.
    // This is to avoid having to create multiple buffers for the scene.
    // The scene info buffer is used to pass the scene information to the shader.
    // The mesh buffer is used to pass the mesh information to the shader.
    // The instance buffer is used to pass the instance information to the shader.
    // The material buffer is used to pass the material information to the shader.
    void createGltfSceneInfoBuffer(GltfSceneResource& scene_resource, StagingUploader& staging_uploader) {
        ResourceAllocator* allocator = staging_uploader.getResourceAllocator();

        // Create all mesh buffers
        allocator->createBuffer(scene_resource.b_meshes, std::span(scene_resource.meshes).size_bytes(), VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT);
        staging_uploader.appendBuffer(scene_resource.b_meshes, 0, std::span<const shaderio::GltfMesh>(scene_resource.meshes));

        // Create all instance buffers
        allocator->createBuffer(scene_resource.b_instances, std::span(scene_resource.instances).size_bytes(), VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT);
        staging_uploader.appendBuffer(scene_resource.b_instances, 0, std::span<const shaderio::GltfInstance>(scene_resource.instances));

        // Create all material buffers
        allocator->createBuffer(scene_resource.b_materials, std::span(scene_resource.materials).size_bytes(), VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT);
        staging_uploader.appendBuffer(scene_resource.b_materials, 0, std::span<const shaderio::GltfMetallicRoughness>(scene_resource.materials));

        // Create the scene info buffer
        allocator->createBuffer(scene_resource.b_scene_info,
                                std::span<const shaderio::GltfSceneInfo>(&scene_resource.scene_info, 1).size_bytes(),
                                VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT);
        staging_uploader.appendBuffer(scene_resource.b_scene_info, 0, std::span<const shaderio::GltfSceneInfo>(&scene_resource.scene_info, 1));
    }

} // namespace vk_test
