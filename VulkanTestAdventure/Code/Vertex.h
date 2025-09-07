#pragma once

struct Vertex {
    glm::vec3 position = glm::vec3();
    glm::vec3 color = glm::vec3();
    glm::vec2 texture_coord = glm::vec2();

    static constexpr vk::VertexInputBindingDescription getBindingDescription() { return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex }; }

    static constexpr auto getAttributeDescriptions() {
        return std::array{
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat   , offsetof(Vertex, texture_coord))
        };
    }

    Vertex() = default;
    ~Vertex() = default;

    constexpr Vertex(const glm::vec3& position, const glm::vec3& color, const glm::vec2& texture_coord)noexcept : position(position), color(color), texture_coord(texture_coord) {}
};

static std::vector<Vertex> VERTICES = {
    // Front face
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 0
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 1
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 2
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 3

    // Back face
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // 4
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, // 5
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // 6
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, // 7

    // Left face
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 8  (4 - повтор)
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 9  (0 - повтор)
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 10 (3 - повтор)
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 11 (7 - повтор)

    // Right face
    {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 12 (1 - повтор)
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 13 (5 - повтор)
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 14 (6 - повтор)
    {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 15 (2 - повтор)

    // Top face
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 16 (3 - повтор)
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 17 (2 - повтор)
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 18 (6 - повтор)
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}, // 19 (7 - повтор)

    // Bottom face
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // 20 (4 - повтор)
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}, // 21 (5 - повтор)
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 22 (1 - повтор)
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}  // 23 (0 - повтор)
};


static std::vector<uint16_t> INDICES = {
    // Front face
    0,  1,  2,
    2,  3,  0,

    // Back face
    4,  5,  6,
    6,  7,  4,

    // Left face
    8,  9,  10,
    10, 11, 8,

    // Right face
    12, 13, 14,
    14, 15, 12,

    // Top face
    16, 17, 18,
    18, 19, 16,

    // Bottom face
    20, 21, 22,
    22, 23, 20
};