
/*
 * Copyright (c) 2019-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2019-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
* Modified by Farrakh
* 2025
*/

#ifndef IO_GLTF_H
#define IO_GLTF_H

#ifdef __cplusplus
#define CHECK_STRUCT_ALIGNMENT(_s) static_assert(sizeof(_s) % 8 == 0);
#elif defined(__SLANG__)
#define CHECK_STRUCT_ALIGNMENT(_s)
#else
#define CHECK_STRUCT_ALIGNMENT(_s)

// This is a utility to define a buffer reference in GLSL.
// Usage: declare the buffer reference type with: BUFFER_REF_DECL(type), where type is the type of the buffer (vec3, float, Material).
// Then use the buffer reference in the shader with: BUFFER_REF(type, address), where address is the address of the buffer in the shader.
#define BUFFER_REF_DECL(_type)                              \
    layout(buffer_reference, scalar) buffer _type##Buffer { \
        _type o[];                                          \
    };

#define BUFFER_REF(_type, _addr) _type##Buffer(_addr).o

#endif

#include "../Shaders/sky_io.h.slang"

NAMESPACE_SHADERIO_BEGIN()
// GLTF
struct BufferView {
    uint32_t offset;      // Offset in the buffer where the data starts (in bytes)
    uint32_t count;       // Number of elements in the buffer view
    uint32_t byte_stride; // Stride in bytes between consecutive elements (0 if tightly packed)
};

struct TriangleMesh {
    BufferView indices;    // Index buffer view
    BufferView positions;  // Position buffer view (vec3)
    BufferView normals;    // Normal buffer view (vec3)
    BufferView color_vert; // color at vertices (vec4, optional)
    BufferView tex_coords; // texture coordinates buffer view (vec2, optional)
    BufferView tangents;   // tangents buffer view (vec4, optional)
};

struct GltfMetallicRoughness {
    float4 base_color_factor;        // Base color factor (RGBA)
    float  metallic_factor;          // Metallic factor (0.0 = non-metallic, 1.0 = metallic)
    float  roughness_factor;         // Roughness factor (0.0 = smooth, 1.0 = rough)
    int    base_color_texture_index; // Index of the base color texture in the GLTF file (optional)
};

struct GltfMesh {
    uint8_t*     gltf_buffer = nullptr; // Buffer to the data (index, position, normal, ...)
    TriangleMesh tri_mesh{};            // Mesh data
    int          index_type{};          // Index type (uint16_t or uint32_t)
};

enum GltfLightType {
    ePoint       = 0, // Point light type
    eSpot        = 1, // Spot light type
    eDirectional = 2  // Directional light type
};

struct GltfPunctual {
    float3 position;   // Position of the punctual light in world space
    float  intensity;  // Intensity of the light
    float3 direction;  // Direction of the light (for spot and directional lights)
    int    type;       // Type of the light (0 = point, 1 = spot, 2 = directional)
    float3 color;      // Color of the light (RGB)
    float  cone_angle; // Cone angle for spot lights (in radians, 0 for point and directional lights)
};

struct GltfInstance {
    float4x4 transform{};      // Transform matrix for the instance (local to world)
    uint32_t material_index{}; // Material properties for the instance
    uint32_t mesh_index{};     // Index of the mesh in the GltfMesh vector
};
CHECK_STRUCT_ALIGNMENT(GltfInstance)

struct GltfSceneInfo {
    float4x4               view_proj_matrix;   // View projection matrix for the scene
    float4x4               proj_inv_matrix;    // Inverse projection matrix for the scene
    float4x4               view_inv_matrix;    // Inverse view matrix for the scene
    float3                 camera_position;    // Camera position in world space
    int                    use_sky;            // Whether to use the sky rendering
    float3                 background_color;   // Background color of the scene (used when not using sky)
    int                    num_lights;         // Number of punctual lights in the scene (up to 2)
    GltfInstance*          instances;          // Address of the instance buffer containing GltfInstance data
    GltfMesh*              meshes;             // Address of the mesh buffer containing GltfMesh data
    GltfMetallicRoughness* materials;          // Material properties for the instance
    GltfPunctual           punctual_lights[2]; // Array of punctual lights in the scene (up to 2)
    SkySimpleParameters    sky_simple_param;   // Parameters for the sky rendering
};
CHECK_STRUCT_ALIGNMENT(GltfSceneInfo)

NAMESPACE_SHADERIO_END()
#endif // IO_GLTF_H
