#pragma once

struct Vertex {
    glm::vec2 position = glm::vec2();
    glm::vec3 color = glm::vec3();
    glm::vec2 texture_coord = glm::vec2();

    static constexpr vk::VertexInputBindingDescription getBindingDescription() { return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex }; }

    static constexpr auto getAttributeDescriptions() {
        return std::array{
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, position)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texture_coord))
        };
    }

    Vertex() = default;
    ~Vertex() = default;

    Vertex(const glm::vec2& position, const glm::vec3& color, const glm::vec2& texture_coord)noexcept : position(position), color(color), texture_coord(texture_coord) {}
};

static std::vector<Vertex> VERTICES{
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

static std::vector<uint16_t> INDICES = {
    0, 1, 2, 2, 3, 0
};