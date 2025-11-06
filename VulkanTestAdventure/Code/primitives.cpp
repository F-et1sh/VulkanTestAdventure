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

#include "pch.h"
#include "primitives.hpp"

#include "hash_operations.hpp"

namespace vk_test {
    static uint32_t addPos(PrimitiveMesh& mesh, glm::vec3 p) {
        PrimitiveVertex v{};
        v.pos = p;
        mesh.vertices.emplace_back(v);
        return static_cast<uint32_t>(mesh.vertices.size()) - 1;
    }

    static void addTriangle(PrimitiveMesh& mesh, uint32_t a, uint32_t b, uint32_t c) {
        mesh.triangles.push_back({ { a, b, c } });
    }

    static void addTriangle(PrimitiveMesh& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        mesh.triangles.push_back({ { addPos(mesh, a), addPos(mesh, b), addPos(mesh, c) } });
    }

    static void generateFacetedNormals(PrimitiveMesh& mesh) {
        auto num_indices = static_cast<int>(mesh.triangles.size());
        for (int i = 0; i < num_indices; i++) {
            auto& v0 = mesh.vertices[mesh.triangles[i].indices[0]];
            auto& v1 = mesh.vertices[mesh.triangles[i].indices[1]];
            auto& v2 = mesh.vertices[mesh.triangles[i].indices[2]];

            glm::vec3 n = glm::normalize(glm::cross(glm::normalize(v1.pos - v0.pos), glm::normalize(v2.pos - v0.pos)));

            v0.nrm = n;
            v1.nrm = n;
            v2.nrm = n;
        }
    }

    // Function to generate texture coordinates
    static void generateTexCoords(PrimitiveMesh& mesh) {
        for (auto& vertex : mesh.vertices) {
            glm::vec3 n = normalize(vertex.pos);
            float     u = 0.5F + (std::atan2(n.z, n.x) / (2.0F * float(M_PI)));
            float     v = 0.5F - (std::asin(n.y) / float(M_PI));
            vertex.tex  = { u, v };
        }
    }

    // Generates a tetrahedron mesh (four triangular faces)
    PrimitiveMesh createTetrahedron() {
        PrimitiveMesh mesh;

        // choose coordinates on the unit sphere
        float a = 1.0F / 3.0F;
        float b = sqrt(8.0F / 9.0F);
        float c = sqrt(2.0F / 9.0F);
        float d = sqrt(2.0F / 3.0F);

        // 4 vertices
        glm::vec3 v0 = glm::vec3{ 0.0F, 1.0F, 0.0F } * 0.5F;
        glm::vec3 v1 = glm::vec3{ -c, -a, d } * 0.5F;
        glm::vec3 v2 = glm::vec3{ -c, -a, -d } * 0.5F;
        glm::vec3 v3 = glm::vec3{ b, -a, 0.0F } * 0.5F;

        // 4 triangles
        addTriangle(mesh, v0, v2, v1);
        addTriangle(mesh, v0, v3, v2);
        addTriangle(mesh, v0, v1, v3);
        addTriangle(mesh, v3, v1, v2);

        generateFacetedNormals(mesh);
        generateTexCoords(mesh);

        return mesh;
    }

    // Generates an icosahedron mesh (twenty equilateral triangular faces)
    PrimitiveMesh createIcosahedron() {
        PrimitiveMesh mesh;

        float sq5 = sqrt(5.0F);
        float a   = 2.0F / (1.0F + sq5);
        float b   = sqrt((3.0F + sq5) / (1.0F + sq5));
        a /= b;
        float r = 0.5F;

        std::vector<glm::vec3> v;
        v.emplace_back(0.0F, r * a, r / b);
        v.emplace_back(0.0F, r * a, -r / b);
        v.emplace_back(0.0F, -r * a, r / b);
        v.emplace_back(0.0F, -r * a, -r / b);
        v.emplace_back(r * a, r / b, 0.0F);
        v.emplace_back(r * a, -r / b, 0.0F);
        v.emplace_back(-r * a, r / b, 0.0F);
        v.emplace_back(-r * a, -r / b, 0.0F);
        v.emplace_back(r / b, 0.0F, r * a);
        v.emplace_back(r / b, 0.0F, -r * a);
        v.emplace_back(-r / b, 0.0F, r * a);
        v.emplace_back(-r / b, 0.0F, -r * a);

        addTriangle(mesh, v[1], v[6], v[4]);
        addTriangle(mesh, v[0], v[4], v[6]);
        addTriangle(mesh, v[0], v[10], v[2]);
        addTriangle(mesh, v[0], v[2], v[8]);
        addTriangle(mesh, v[1], v[9], v[3]);
        addTriangle(mesh, v[1], v[3], v[11]);
        addTriangle(mesh, v[2], v[7], v[5]);
        addTriangle(mesh, v[3], v[5], v[7]);
        addTriangle(mesh, v[6], v[11], v[10]);
        addTriangle(mesh, v[7], v[10], v[11]);
        addTriangle(mesh, v[4], v[8], v[9]);
        addTriangle(mesh, v[5], v[9], v[8]);
        addTriangle(mesh, v[0], v[6], v[10]);
        addTriangle(mesh, v[0], v[8], v[4]);
        addTriangle(mesh, v[1], v[11], v[6]);
        addTriangle(mesh, v[1], v[4], v[9]);
        addTriangle(mesh, v[3], v[7], v[11]);
        addTriangle(mesh, v[3], v[9], v[5]);
        addTriangle(mesh, v[2], v[10], v[7]);
        addTriangle(mesh, v[2], v[5], v[8]);

        generateFacetedNormals(mesh);
        generateTexCoords(mesh);

        return mesh;
    }

    // Generates an octahedron mesh (eight faces), this is like two four-sided pyramids placed base to base.
    PrimitiveMesh createOctahedron() {
        PrimitiveMesh mesh;

        std::vector<glm::vec3> v;
        v.emplace_back(0.5F, 0.0F, 0.0F);
        v.emplace_back(-0.5F, 0.0F, 0.0F);
        v.emplace_back(0.0F, 0.5F, 0.0F);
        v.emplace_back(0.0F, -0.5F, 0.0F);
        v.emplace_back(0.0F, 0.0F, 0.5F);
        v.emplace_back(0.0F, 0.0F, -0.5F);

        addTriangle(mesh, v[0], v[2], v[4]);
        addTriangle(mesh, v[0], v[4], v[3]);
        addTriangle(mesh, v[0], v[5], v[2]);
        addTriangle(mesh, v[0], v[3], v[5]);
        addTriangle(mesh, v[1], v[4], v[2]);
        addTriangle(mesh, v[1], v[3], v[4]);
        addTriangle(mesh, v[1], v[5], v[3]);
        addTriangle(mesh, v[2], v[5], v[1]);

        generateFacetedNormals(mesh);
        generateTexCoords(mesh);

        return mesh;
    }

    // Generates a flat plane mesh with the specified number of steps, width, and depth.
    // The plane is essentially a grid with the specified number of subdivisions (steps)
    // in both the X and Z directions. It creates vertices, normals, and texture coordinates
    // for each point on the grid and forms triangles to create the plane's surface.
    PrimitiveMesh createPlane(int steps, float width, float depth) {
        PrimitiveMesh mesh;

        float increment = 1.0F / static_cast<float>(steps);
        for (int sz = 0; sz <= steps; sz++) {
            for (int sx = 0; sx <= steps; sx++) {
                PrimitiveVertex v{};

                v.pos = glm::vec3(-0.5F + (static_cast<float>(sx) * increment), 0.0F, -0.5F + (static_cast<float>(sz) * increment));
                v.pos *= glm::vec3(width, 1.0F, depth);
                v.nrm = glm::vec3(0.0F, 1.0F, 0.0F);
                v.tex = glm::vec2(static_cast<float>(sx) / static_cast<float>(steps),
                                  static_cast<float>(steps - sz) / static_cast<float>(steps));
                mesh.vertices.emplace_back(v);
            }
        }

        for (int sz = 0; sz < steps; sz++) {
            for (int sx = 0; sx < steps; sx++) {
                addTriangle(mesh, sx + (sz * (steps + 1)), sx + 1 + ((sz + 1) * (steps + 1)), sx + 1 + (sz * (steps + 1)));
                addTriangle(mesh, sx + (sz * (steps + 1)), sx + ((sz + 1) * (steps + 1)), sx + 1 + ((sz + 1) * (steps + 1)));
            }
        }

        return mesh;
    }

    // Generates a cube mesh with the specified width, height, and depth
    // Start with 8 vertex, 6 normal and 4 uv, then 12 triangles and 24
    // unique PrimitiveVertex
    PrimitiveMesh createCube(float width /*= 1*/, float height /*= 1*/, float depth /*= 1*/) {
        PrimitiveMesh mesh;

        glm::vec3              s   = glm::vec3(width, height, depth) * 0.5F;
        std::vector<glm::vec3> pnt = { { -s.x, -s.y, -s.z }, { -s.x, -s.y, s.z }, { -s.x, s.y, -s.z }, { -s.x, s.y, s.z }, { s.x, -s.y, -s.z }, { s.x, -s.y, s.z }, { s.x, s.y, -s.z }, { s.x, s.y, s.z } };
        std::vector<glm::vec3> nrm = { { -1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, 1.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, -1.0F }, { 0.0F, -1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } };
        std::vector<glm::vec2> uv  = { { 0.0F, 0.0F }, { 0.0F, 1.0F }, { 1.0F, 1.0F }, { 1.0F, 0.0F } };

        // cube topology
        std::vector<std::vector<int>> cube_polygons = { { 0, 1, 3, 2 }, { 1, 5, 7, 3 }, { 5, 4, 6, 7 }, { 4, 0, 2, 6 }, { 4, 5, 1, 0 }, { 2, 3, 7, 6 } };

        for (int i = 0; i < 6; ++i) {
            auto index = static_cast<int>(mesh.vertices.size());
            for (int j = 0; j < 4; ++j) {
                mesh.vertices.push_back({ pnt[cube_polygons[i][j]], nrm[i], uv[j] });
            }
            addTriangle(mesh, index, index + 1, index + 2);
            addTriangle(mesh, index, index + 2, index + 3);
        }

        return mesh;
    }

    // Generates a UV-sphere mesh with the specified radius, number of sectors (horizontal subdivisions)
    // and stacks (vertical subdivisions). It uses latitude-longitude grid generation to create vertices
    // with proper positions, normals, and texture coordinates.
    PrimitiveMesh createSphereUv(float radius, int sectors, int stacks) {
        PrimitiveMesh mesh;

        float omega{ 0.0F };              // rotation around the X axis
        float phi{ 0.0F };                // rotation around the Y axis
        float length_inv = 1.0F / radius; // vertex normal

        const float math_pi     = static_cast<float>(M_PI);
        float       sector_step = 2.0F * math_pi / static_cast<float>(sectors);
        float       stack_step  = math_pi / static_cast<float>(stacks);
        float       sector_angle{ 0.0F };
        float       stack_angle{ 0.0F };

        for (int i = 0; i <= stacks; ++i) {
            stack_angle = math_pi / 2.0F - static_cast<float>(i) * stack_step; // starting from pi/2 to -pi/2
            phi         = radius * cosf(stack_angle);                          // r * cos(u)
            omega       = radius * sinf(stack_angle);                          // r * sin(u)

            // add (sectorCount+1) vertices per stack
            // the first and last vertices have same position and normal, but different tex coords
            for (int j = 0; j <= sectors; ++j) {
                PrimitiveVertex v{};

                sector_angle = static_cast<float>(j) * sector_step; // starting from 0 to 2pi

                // vertex position (x, y, z)
                v.pos.x = phi * cosf(sector_angle); // r * cos(u) * cos(v)
                v.pos.z = phi * sinf(sector_angle); // r * cos(u) * sin(v)
                v.pos.y = omega;

                // normalized vertex normal
                v.nrm = v.pos * length_inv;

                // vertex tex coord (s, t) range between [0, 1]
                v.tex.x = 1.0F - static_cast<float>(j) / static_cast<float>(sectors);
                v.tex.y = static_cast<float>(i) / static_cast<float>(stacks);

                mesh.vertices.emplace_back(v);
            }
        }

        // indices
        //  k2---k2+1
        //  | \  |
        //  |  \ |
        //  k1---k1+1
        int k1{ 0 };
        int k2{ 0 };
        for (int i = 0; i < stacks; ++i) {
            k1 = i * (sectors + 1); // beginning of current stack
            k2 = k1 + sectors + 1;  // beginning of next stack

            for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
                // 2 triangles per sector excluding 1st and last stacks
                if (i != 0) {
                    addTriangle(mesh, k1, k1 + 1, k2); // k1---k2---k1+1
                }

                if (i != (stacks - 1)) {
                    addTriangle(mesh, k1 + 1, k2 + 1, k2); // k1+1---k2---k2+1
                }
            }
        }

        return mesh;
    }

    // Function to create a cone
    // radius   :Adjust this to change the size of the cone
    // height   :Adjust this to change the height of the cone
    // segments :Adjust this for the number of segments forming the base circle
    PrimitiveMesh createConeMesh(float radius, float height, int segments) {
        PrimitiveMesh mesh;

        float half_height = height * 0.5F;

        const float math_pi     = static_cast<float>(M_PI);
        float       sector_step = 2.0F * math_pi / static_cast<float>(segments);
        float       sector_angle{ 0.0F };

        // length of the flank of the cone
        float flank_len = sqrtf((radius * radius) + 1.0F);
        // unit vector along the flank of the cone
        float cone_x = radius / flank_len;
        float cone_y = -1.0F / flank_len;

        glm::vec3 tip = { 0.0F, half_height, 0.0F };

        // Sides
        for (int i = 0; i <= segments; ++i) {
            PrimitiveVertex v{};
            sector_angle = static_cast<float>(i) * sector_step;

            // Position
            v.pos.x = radius * cosf(sector_angle); // r * cos(u) * cos(v)
            v.pos.z = radius * sinf(sector_angle); // r * cos(u) * sin(v)
            v.pos.y = -half_height;
            // Normal
            v.nrm.x = -cone_y * cosf(sector_angle);
            v.nrm.y = cone_x;
            v.nrm.z = -cone_y * sinf(sector_angle);
            // TexCoord
            v.tex.x = static_cast<float>(i) / static_cast<float>(segments);
            v.tex.y = 0.0F;
            mesh.vertices.emplace_back(v);

            // Tip point
            v.pos = tip;
            // Normal
            sector_angle += 0.5F * sector_step; // Half way to next triangle
            v.nrm.x = -cone_y * cosf(sector_angle);
            v.nrm.y = cone_x;
            v.nrm.z = -cone_y * sinf(sector_angle);
            // TexCoord
            v.tex.x += 0.5F / static_cast<float>(segments);
            v.tex.y = 1.0F;

            mesh.vertices.emplace_back(v);
        }

        for (int j = 0; j < segments; ++j) {
            int k1 = j * 2;
            addTriangle(mesh, k1, k1 + 1, k1 + 2);
        }

        // Bottom plate (normal are different)
        for (int i = 0; i <= segments; ++i) {
            PrimitiveVertex v{};
            sector_angle = static_cast<float>(i) * sector_step; // starting from 0 to 2pi

            v.pos.x = radius * cosf(sector_angle); // r * cos(u) * cos(v)
            v.pos.z = radius * sinf(sector_angle); // r * cos(u) * sin(v)
            v.pos.y = -half_height;
            //
            v.nrm = { 0.0F, -1.0F, 0.0F };
            //
            v.tex.x = static_cast<float>(i) / static_cast<float>(segments);
            v.tex.y = 0.0F;
            mesh.vertices.emplace_back(v);

            v.pos = -tip;
            v.tex.x += 0.5F / static_cast<float>(segments);
            v.tex.y = 1.0F;
            mesh.vertices.emplace_back(v);
        }

        for (int j = 0; j < segments; ++j) {
            int k1 = (j + segments + 1) * 2;
            addTriangle(mesh, k1, k1 + 2, k1 + 1);
        }

        return mesh;
    }

    // Generates a sphere mesh with the specified radius and subdivisions (level of detail).
    // It uses the icosahedron subdivision technique to iteratively refine the mesh by
    // subdividing triangles into smaller triangles to approximate a more spherical shape.
    // It calculates vertex positions, normals, and texture coordinates for each vertex
    // and constructs triangles accordingly.
    // Note: There will be duplicated vertices with this method.
    //       Use removeDuplicateVertices to avoid duplicated vertices.
    PrimitiveMesh createSphereMesh(float radius, int subdivisions) {

        const float            t        = (1.0F + std::sqrt(5.0F)) / 2.0F; // Golden ratio
        std::vector<glm::vec3> vertices = { { -1, t, 0 }, { 1, t, 0 }, { -1, -t, 0 }, { 1, -t, 0 }, { 0, -1, t }, { 0, 1, t }, { 0, -1, -t }, { 0, 1, -t }, { t, 0, -1 }, { t, 0, 1 }, { -t, 0, -1 }, { -t, 0, 1 } };

        // Function to calculate the midpoint between two vertices
        auto midpoint = [](const glm::vec3& v1, const glm::vec3& v2) { return (v1 + v2) * 0.5F; };

        auto tex_coord = [](const glm::vec3& v1) {
            return glm::vec2{ 0.5F + (std::atan2(v1.z, v1.x) / (2 * M_PI)), 0.5F - (std::asin(v1.y) / M_PI) };
        };

        std::vector<PrimitiveVertex> primitive_vertices;
        for (const auto& vertex : vertices) {
            glm::vec3 n = normalize(vertex);
            primitive_vertices.push_back({ n * radius, n, tex_coord(n) });
        }

        std::vector<PrimitiveTriangle> triangles = { { { 0, 11, 5 } }, { { 0, 5, 1 } }, { { 0, 1, 7 } }, { { 0, 7, 10 } }, { { 0, 10, 11 } }, { { 1, 5, 9 } }, { { 5, 11, 4 } }, { { 11, 10, 2 } }, { { 10, 7, 6 } }, { { 7, 1, 8 } }, { { 3, 9, 4 } }, { { 3, 4, 2 } }, { { 3, 2, 6 } }, { { 3, 6, 8 } }, { { 3, 8, 9 } }, { { 4, 9, 5 } }, { { 2, 4, 11 } }, { { 6, 2, 10 } }, { { 8, 6, 7 } }, { { 9, 8, 1 } } };

        for (int i = 0; i < subdivisions; ++i) {
            std::vector<PrimitiveTriangle> sub_triangles;
            for (const auto& tri : triangles) {
                // Subdivide each triangle into 4 sub-triangles
                glm::vec3 mid1 = midpoint(primitive_vertices[tri.indices[0]].pos, primitive_vertices[tri.indices[1]].pos);
                glm::vec3 mid2 = midpoint(primitive_vertices[tri.indices[1]].pos, primitive_vertices[tri.indices[2]].pos);
                glm::vec3 mid3 = midpoint(primitive_vertices[tri.indices[2]].pos, primitive_vertices[tri.indices[0]].pos);

                glm::vec3 mid1_normalized = normalize(mid1);
                glm::vec3 mid2_normalized = normalize(mid2);
                glm::vec3 mid3_normalized = normalize(mid3);

                glm::vec2 mid1_uv = tex_coord(mid1_normalized);
                glm::vec2 mid2_uv = tex_coord(mid2_normalized);
                glm::vec2 mid3_uv = tex_coord(mid3_normalized);

                primitive_vertices.push_back({ mid1_normalized * radius, mid1_normalized, mid1_uv });
                primitive_vertices.push_back({ mid2_normalized * radius, mid2_normalized, mid2_uv });
                primitive_vertices.push_back({ mid3_normalized * radius, mid3_normalized, mid3_uv });

                uint32_t m1 = static_cast<uint32_t>(primitive_vertices.size()) - 3U;
                uint32_t m2 = m1 + 1U;
                uint32_t m3 = m2 + 1U;

                // Create 4 new triangles from the subdivided triangle
                sub_triangles.push_back({ { tri.indices[0], m1, m3 } });
                sub_triangles.push_back({ { m1, tri.indices[1], m2 } });
                sub_triangles.push_back({ { m2, tri.indices[2], m3 } });
                sub_triangles.push_back({ { m1, m2, m3 } });
            }

            triangles = std::move(sub_triangles);
        }

        return PrimitiveMesh{ std::move(primitive_vertices), std::move(triangles) };
    }

    // Generates a torus mesh, which is a 3D geometric shape resembling a donut
    // majorRadius: This represents the distance from the center of the torus to the center of the tube (the larger circle's radius).
    // minorRadius: This represents the radius of the tube (the smaller circle's radius).
    // majorSegments: The number of segments used to approximate the larger circle that forms the torus.
    // minorSegments: The number of segments used to approximate the smaller circle (tube) within the torus.
    vk_test::PrimitiveMesh createTorusMesh(float major_radius, float minor_radius, int major_segments, int minor_segments) {
        vk_test::PrimitiveMesh mesh;

        float major_step = 2.0F * float(M_PI) / float(major_segments);
        float minor_step = 2.0F * float(M_PI) / float(minor_segments);

        for (int i = 0; i <= major_segments; ++i) {
            float     angle1 = i * major_step;
            glm::vec3 center = { major_radius * std::cos(angle1), 0.0F, major_radius * std::sin(angle1) };

            for (int j = 0; j <= minor_segments; ++j) {
                float     angle2   = j * minor_step;
                glm::vec3 position = { center.x + (minor_radius * std::cos(angle2) * std::cos(angle1)), minor_radius * std::sin(angle2), center.z + (minor_radius * std::cos(angle2) * std::sin(angle1)) };

                glm::vec3 normal = { std::cos(angle2) * std::cos(angle1), std::sin(angle2), std::cos(angle2) * std::sin(angle1) };

                glm::vec2 tex_coord = { static_cast<float>(i) / major_segments, static_cast<float>(j) / minor_segments };
                mesh.vertices.push_back({ position, normal, tex_coord });
            }
        }

        for (int i = 0; i < major_segments; ++i) {
            for (int j = 0; j < minor_segments; ++j) {
                uint32_t idx1 = (i * (minor_segments + 1)) + j;
                uint32_t idx2 = ((i + 1) * (minor_segments + 1)) + j;
                uint32_t idx3 = idx1 + 1;
                uint32_t idx4 = idx2 + 1;

                mesh.triangles.push_back({ { idx1, idx3, idx2 } });
                mesh.triangles.push_back({ { idx3, idx4, idx2 } });
            }
        }

        return mesh;
    }

    //------------------------------------------------------------------------
    // Create a vector of nodes that represent the Menger Sponge
    // Nodes have a different translation and scale, which can be used with
    // different objects.
    std::vector<vk_test::Node> mengerSpongeNodes(int level, float probability, int seed) {
        std::mt19937                          rng(seed);
        std::uniform_real_distribution<float> dist(0.0F, 1.0F);

        struct MengerSponge {
            glm::vec3 m_topLeftFront;
            float     m_size;

            void split(std::vector<MengerSponge>& cubes) {
                float     size           = m_size / 3.F;
                glm::vec3 top_left_front = m_topLeftFront;
                for (int x = 0; x < 3; x++) {
                    top_left_front[0] = m_topLeftFront[0] + static_cast<float>(x) * size;
                    for (int y = 0; y < 3; y++) {
                        if (x == 1 && y == 1) {
                            continue;
                        }
                        top_left_front[1] = m_topLeftFront[1] + static_cast<float>(y) * size;
                        for (int z = 0; z < 3; z++) {
                            if (x == 1 && z == 1) {
                                continue;
                            }
                            if (y == 1 && z == 1) {
                                continue;
                            }

                            top_left_front[2] = m_topLeftFront[2] + static_cast<float>(z) * size;
                            cubes.push_back({ top_left_front, size });
                        }
                    }
                }
            }

            void splitProb(std::vector<MengerSponge>& cubes, float prob, std::mt19937& rng, std::uniform_real_distribution<float>& dist) {
                float     size           = m_size / 3.F;
                glm::vec3 top_left_front = m_topLeftFront;
                for (int x = 0; x < 3; x++) {
                    top_left_front[0] = m_topLeftFront[0] + static_cast<float>(x) * size;
                    for (int y = 0; y < 3; y++) {
                        top_left_front[1] = m_topLeftFront[1] + static_cast<float>(y) * size;
                        for (int z = 0; z < 3; z++) {
                            float sample = dist(rng);
                            if (sample > prob) {
                                continue;
                            }
                            top_left_front[2] = m_topLeftFront[2] + static_cast<float>(z) * size;
                            cubes.push_back({ top_left_front, size });
                        }
                    }
                }
            }
        };

        // Starting element
        MengerSponge element = { glm::vec3(-0.5, -0.5, -0.5), 1.F };

        std::vector<MengerSponge> elements1 = { element };
        std::vector<MengerSponge> elements2 = {};

        auto* previous = &elements1;
        auto* next     = &elements2;

        for (int i = 0; i < level; i++) {
            for (MengerSponge& c : *previous) {
                if (probability < 0.F) {
                    c.split(*next);
                }
                else {
                    c.splitProb(*next, probability, rng, dist);
                }
            }
            auto* temp = previous;
            previous   = next;
            next       = temp;
            next->clear();
        }

        std::vector<vk_test::Node> nodes;
        for (MengerSponge& c : *previous) {
            vk_test::Node node{};
            node.translation = c.m_topLeftFront;
            node.scale       = glm::vec3(c.m_size);
            node.mesh        = 0; // default to the first mesh
            nodes.push_back(node);
        }

        return nodes;
    }

    //-------------------------------------------------------------------------------------------------
    // Create a list of nodes where the seeds have the position similar as in a sun flower
    // and the seeds grow slightly the further they are from the center.
    std::vector<vk_test::Node> sunflower(int seeds) {
        constexpr double golden_ratio = glm::golden_ratio<double>();

        std::vector<vk_test::Node> flower;
        for (int i = 1; i <= seeds; ++i) {
            double r     = pow(i, golden_ratio) / seeds;
            double theta = 2 * glm::pi<double>() * golden_ratio * i;

            vk_test::Node seed;
            seed.translation = glm::vec3(r * sin(theta), 0, r * cos(theta));
            seed.scale       = glm::vec3(10.0F * i / (1.0F * seeds));
            seed.mesh        = 0;

            flower.push_back(seed);
        }
        return flower;
    }

    //---------------------------------------------------------------------------
    // Merge all nodes meshes into a single one
    // - nodes: the nodes to merge
    // - meshes: the mesh array that the nodes is referring to
    vk_test::PrimitiveMesh mergeNodes(const std::vector<vk_test::Node>& nodes, const std::vector<vk_test::PrimitiveMesh> meshes) {
        vk_test::PrimitiveMesh result_mesh;

        // Find how many triangles and vertices the merged mesh will have
        size_t nb_triangles = 0;
        size_t nb_vertices  = 0;
        for (const auto& n : nodes) {
            nb_triangles += meshes[n.mesh].triangles.size();
            nb_vertices += meshes[n.mesh].vertices.size();
        }
        result_mesh.triangles.reserve(nb_triangles);
        result_mesh.vertices.reserve(nb_vertices);

        // Merge all nodes meshes into a single one
        for (const auto& n : nodes) {
            const glm::mat4 mat = value_type::localMatrix();

            uint32_t                      t_index = static_cast<uint32_t>(result_mesh.vertices.size());
            const vk_test::PrimitiveMesh& mesh    = meshes[n.mesh];

            for (auto v : mesh.vertices) {
                v.pos = glm::vec3(mat * glm::vec4(v.pos, 1));
                result_mesh.vertices.push_back(v);
            }
            for (auto t : mesh.triangles) {
                t.indices += t_index;
                result_mesh.triangles.push_back(t);
            }
        }

        return result_mesh;
    }

    // Takes a 3D mesh as input and modifies its vertices by adding random displacements within a
    // specified `amplitude` range to create a wobbling effect. The intensity of the wobbling effect
    // can be controlled by adjusting the `amplitude` parameter.
    // The function returns the modified mesh.
    vk_test::PrimitiveMesh wobblePrimitive(const vk_test::PrimitiveMesh& mesh, float amplitude) {
        // Seed the random number generator with a random device
        std::random_device rd;
        std::mt19937       gen(rd());

        // Define the range for the random number generation (-1.0 to 1.0)
        std::uniform_real_distribution<float> distribution(-1.0, 1.0);

        // Our random function
        auto rand = [&] { return distribution(gen); };

        std::vector<PrimitiveVertex> new_vertices;
        for (const auto& vertex : mesh.vertices) {
            glm::vec3 original_position = vertex.pos;
            glm::vec3 displacement      = glm::vec3(rand(), rand(), rand());
            displacement *= amplitude;
            glm::vec3 new_position = original_position + displacement;

            new_vertices.push_back({ new_position, vertex.nrm, vertex.tex });
        }

        return { std::move(new_vertices), std::move(mesh.triangles) };
    }

    // Takes a 3D mesh as input and returns a new mesh with duplicate vertices removed.
    // This function iterates through each triangle in the original PrimitiveMesh,
    // compares its vertices, and creates a new set of unique vertices in uniqueVertices.
    // We use an unordered_map called vertexIndexMap to keep track of the mapping between
    // the original vertices and their corresponding indices in the uniqueVertices vector.
    PrimitiveMesh removeDuplicateVertices(const PrimitiveMesh& mesh, bool test_normal, bool test_uv) {
        auto hash = [&](const PrimitiveVertex& v) {
            if (test_normal) {
                if (test_uv) {
                    return vk_test::hashVal(v.pos.x, v.pos.y, v.pos.z, v.nrm.x, v.nrm.y, v.nrm.z, v.tex.x, v.tex.y);
                }
                return vk_test::hashVal(v.pos.x, v.pos.y, v.pos.z, v.nrm.x, v.nrm.y, v.nrm.z);
            }
            if (test_uv) {
                return vk_test::hashVal(v.pos.x, v.pos.y, v.pos.z, v.tex.x, v.tex.y);
            }
            return vk_test::hashVal(v.pos.x, v.pos.y, v.pos.z);
        };
        auto equal = [&](const PrimitiveVertex& l, const PrimitiveVertex& r) {
            return (l.pos == r.pos) && (test_normal ? l.nrm == r.nrm : true) && (test_uv ? l.tex == r.tex : true);
        };
        std::unordered_map<PrimitiveVertex, uint32_t, decltype(hash), decltype(equal)> vertex_index_map(0, hash, equal);

        std::vector<PrimitiveVertex>   unique_vertices;
        std::vector<PrimitiveTriangle> unique_triangles;

        for (const auto& triangle : mesh.triangles) {
            PrimitiveTriangle unique_triangle = {};
            for (int i = 0; i < 3; i++) {
                const PrimitiveVertex& vertex = mesh.vertices[triangle.indices[i]];

                // Check if the vertex is already in the uniqueVertices list
                auto it = vertex_index_map.find(vertex);
                if (it == vertex_index_map.end()) {
                    // Vertex not found, add it to uniqueVertices and update the index map
                    uint32_t new_index       = static_cast<uint32_t>(unique_vertices.size());
                    vertex_index_map[vertex] = new_index;
                    unique_vertices.push_back(vertex);
                    unique_triangle.indices[i] = new_index;
                }
                else {
                    // Vertex found, use its index in uniqueVertices
                    unique_triangle.indices[i] = it->second;
                }
            }
            unique_triangles.push_back(unique_triangle);
        }

        // nvprintf("Before: %d vertex, %d triangles\n", mesh.vertices.size(), mesh.triangles.size());
        // nvprintf("After: %d vertex, %d triangles\n", uniqueVertices.size(), uniqueTriangles.size());

        return { std::move(unique_vertices), std::move(unique_triangles) };
    }
} // namespace vk_test
