#pragma once

struct Vertex {
    glm::vec2 position = glm::vec2();
    glm::vec3 color = glm::vec3();

    static vk::VertexInputBindingDescription getBindingDescription() {
        return { 0, sizeof(Vertex), vk::VertexInputRate::eVertex };
    }

    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, position)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color))
        };
    }

    Vertex() = default;
    ~Vertex() = default;

    Vertex(const glm::vec2& position, const glm::vec3& color)noexcept : position(position), color(color) {}
};

static std::vector<Vertex> VERTICES{
    Vertex{glm::vec2{ 0.0f, -0.5f}, glm::vec3{1.0f, 0.0f, 0.0f}},
    Vertex{glm::vec2{ 0.5f,  0.5f}, glm::vec3{0.0f, 0.0f, 1.0f}},
    Vertex{glm::vec2{-0.5f,  0.5f}, glm::vec3{0.0f, 0.0f, 1.0f}}
};