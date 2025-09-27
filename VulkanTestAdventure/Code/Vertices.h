#pragma once

namespace VKTest {
    struct Vertex {
        glm::vec3 position = glm::vec3();
        glm::vec3 color = glm::vec3();
        glm::vec2 texture_coord = glm::vec2();

        static constexpr inline vk::VertexInputBindingDescription getBindingDescription()noexcept {
            static constexpr vk::VertexInputBindingDescription binding_description{
                0,                           // Binding
                sizeof(Vertex),              // Stride
                vk::VertexInputRate::eVertex // Input Rate
            };

            return binding_description;
        }

        static constexpr inline std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
            static constexpr std::array<vk::VertexInputAttributeDescription, 3> attribute_descriptions{
                vk::VertexInputAttributeDescription {
                    0,                                                      // Location
                    0,                                                      // Binding
                    vk::Format::eR32G32B32Sfloat,                           // Format
                    static_cast<uint32_t>(offsetof(Vertex, position))       // Offset
                },
                vk::VertexInputAttributeDescription {
                    1,                                                      // Location
                    0,                                                      // Binding
                    vk::Format::eR32G32B32Sfloat,                           // Format
                    static_cast<uint32_t>(offsetof(Vertex, color))          // Offset
                },
                vk::VertexInputAttributeDescription {
                    2,                                                      // Location
                    0,                                                      // Binding
                    vk::Format::eR32G32Sfloat,                              // Format
                    static_cast<uint32_t>(offsetof(Vertex, texture_coord))  // Offset
                },
            };

            return attribute_descriptions;
        }

        constexpr bool operator==(const Vertex& other)const noexcept { return position == other.position && color == other.color && texture_coord == other.texture_coord; }
    };
}