#pragma once

namespace vk_test {
    struct Vertex {
        glm::vec3 position      = glm::vec3{};
        glm::vec3 color         = glm::vec3{};
        glm::vec2 texture_coord = glm::vec2{};

        static constexpr VkVertexInputBindingDescription getBindingDescription() noexcept {
            return {
                0,                          // Binding
                sizeof(Vertex),             // Stride
                VK_VERTEX_INPUT_RATE_VERTEX // Input Rate
            };
        }

        static constexpr std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            return {
                VkVertexInputAttributeDescription{
                    0,                                                // Location
                    0,                                                // Binding
                    VK_FORMAT_R32G32B32_SFLOAT,                       // Format
                    static_cast<uint32_t>(offsetof(Vertex, position)) // Offset
                },
                VkVertexInputAttributeDescription{
                    1,                                             // Location
                    0,                                             // Binding
                    VK_FORMAT_R32G32B32_SFLOAT,                    // Format
                    static_cast<uint32_t>(offsetof(Vertex, color)) // Offset
                },
                VkVertexInputAttributeDescription{
                    2,                                                     // Location
                    0,                                                     // Binding
                    VK_FORMAT_R32G32_SFLOAT,                               // Format
                    static_cast<uint32_t>(offsetof(Vertex, texture_coord)) // Offset
                },
            };
        }

        constexpr bool operator==(const Vertex& other) const noexcept { return position == other.position && color == other.color && texture_coord == other.texture_coord; }
    };

    const std::vector<Vertex> VERTICES = {
        { { -0.5F, -0.5F, 0.0F }, { 1.0F, 0.0F, 0.0F } },
        { { 0.5F, -0.5F, 0.0F }, { 0.0F, 1.0F, 0.0F } },
        { { 0.5F, 0.5F, 0.0F }, { 0.0F, 0.0F, 1.0F } },
        { { -0.5F, 0.5F, 0.0F }, { 1.0F, 1.0F, 1.0F } }
    };

    const std::vector<uint16_t> INDICES = {
        0, 1, 2, 2, 3, 0
    };
} // namespace vk_test

namespace std {
    template <>
    struct hash<vk_test::Vertex> {
        size_t operator()(vk_test::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texture_coord) << 1);
        }
    };
} // namespace std
