#pragma once

namespace vk_test {
    struct Vertex {
        glm::vec3 position      = glm::vec3();
        glm::vec3 color         = glm::vec3();
        glm::vec2 texture_coord = glm::vec2();

        static constexpr inline vk::VertexInputBindingDescription getBindingDescription() noexcept {
            return {
                0,                           // Binding
                sizeof(Vertex),              // Stride
                vk::VertexInputRate::eVertex // Input Rate
            };
        }

        static constexpr inline std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
            return {
                vk::VertexInputAttributeDescription{
                    0,                                                // Location
                    0,                                                // Binding
                    vk::Format::eR32G32B32Sfloat,                     // Format
                    static_cast<uint32_t>(offsetof(Vertex, position)) // Offset
                },
                vk::VertexInputAttributeDescription{
                    1,                                             // Location
                    0,                                             // Binding
                    vk::Format::eR32G32B32Sfloat,                  // Format
                    static_cast<uint32_t>(offsetof(Vertex, color)) // Offset
                },
                vk::VertexInputAttributeDescription{
                    2,                                                     // Location
                    0,                                                     // Binding
                    vk::Format::eR32G32Sfloat,                             // Format
                    static_cast<uint32_t>(offsetof(Vertex, texture_coord)) // Offset
                },
            };
        }

        constexpr bool operator==(const Vertex& other) const noexcept { return position == other.position && color == other.color && texture_coord == other.texture_coord; }
    };

    const std::vector<Vertex> VERTICES = {
        { { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { { 0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        { { -0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f } }
    };

    const std::vector<uint16_t> INDICES = {
        0, 1, 2, 2, 3, 0
    };
} // namespace vk_test
