/*
 * Copyright (c) 2022-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
* Modified by Farrakh
* 2025
*/

#pragma once

/*-------------------------------------------------------------------------------------------------
# struct `vk_test::PrimitiveMesh`
  - Common primitive type, made of vertices: position, normal and texture coordinates.
  - All primitives are triangles, and each 3 indices is forming a triangle.

# struct `vk_test::Node`
  - Structure to hold a reference to a mesh, with a material and transformation.

Primitives that can be created:
* Tetrahedron
* Icosahedron
* Octahedron
* Plane
* Cube
* SphereUv
* Cone
* SphereMesh
* Torus

Node creator: returns the instance and the position
* MengerSponge
* SunFlower

Other utilities
* mergeNodes
* removeDuplicateVertices
* wobblePrimitive

-------------------------------------------------------------------------------------------------*/

namespace vk_test {
    struct PrimitiveVertex {
        glm::vec3 pos; // Position
        glm::vec3 nrm; // Normal
        glm::vec2 tex; // Texture Coordinates
    };

    struct PrimitiveTriangle {
        glm::uvec3 indices; // vertex indices
    };

    struct PrimitiveMesh {
        std::vector<PrimitiveVertex>   vertices;  // Array of all vertex
        std::vector<PrimitiveTriangle> triangles; // Indices forming triangles
    };

    struct Node {
        glm::vec3 translation{}; //
        glm::quat rotation{};    //
        glm::vec3 scale{ 1.0F }; //
        glm::mat4 matrix{ 1 };   // Added with the above transformations
        int       material{ 0 };
        int       mesh{ -1 };

        static glm::mat4 localMatrix() {
            glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0F), translation);
            glm::mat4 rotation_matrix    = glm::mat4_cast(rotation);
            glm::mat4 scale_matrix       = glm::scale(glm::mat4(1.0F), scale);
            glm::mat4 combined_matrix    = translation_matrix * rotation_matrix * scale_matrix * matrix;
            return combined_matrix;
        }
    };

    PrimitiveMesh createTetrahedron();
    PrimitiveMesh createIcosahedron();
    PrimitiveMesh createOctahedron();
    PrimitiveMesh createPlane(int steps = 1, float width = 1.0F, float depth = 1.0F);
    PrimitiveMesh createCube(float width = 1.0F, float height = 1.0F, float depth = 1.0F);
    PrimitiveMesh createSphereUv(float radius = 0.5F, int sectors = 20, int stacks = 20);
    PrimitiveMesh createConeMesh(float radius = 0.5F, float height = 1.0F, int segments = 16);
    PrimitiveMesh createSphereMesh(float radius = 0.5F, int subdivisions = 3);
    PrimitiveMesh createTorusMesh(float major_radius = 0.5F, float minor_radius = 0.25F, int major_segments = 32, int minor_segments = 16);

    std::vector<Node> mengerSpongeNodes(int level = 3, float probability = -1.F, int seed = 1);
    std::vector<Node> sunflower(int seeds = 3000);

    // Utilities
    PrimitiveMesh mergeNodes(const std::vector<Node>& nodes, std::vector<PrimitiveMesh> meshes);
    PrimitiveMesh removeDuplicateVertices(const PrimitiveMesh& mesh, bool test_normal = true, bool test_uv = true);
    PrimitiveMesh wobblePrimitive(const PrimitiveMesh& mesh, float amplitude = 0.05F);

} // namespace vk_test
