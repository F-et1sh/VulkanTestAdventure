#pragma once
#include "Vertices.h"

namespace VKTest {
    struct UniformBufferObject {
        alignas(16) glm::mat4 model;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 proj;
    };

    // Define a structure to hold per-object data
    struct GameObject {
        // Transform properties
        glm::vec3 position = { 0.0F, 0.0F, 0.0F };
        glm::vec3 rotation = { 0.0F, 0.0F, 0.0F };
        glm::vec3 scale    = { 1.0F, 1.0F, 1.0F };

        // Uniform buffer for this object (one per frame in flight)
        std::vector<VkBuffer>       uniform_buffers;
        std::vector<VkDeviceMemory> uniform_buffers_memory;
        std::vector<void*>          uniform_buffers_mapped;

        // Descriptor sets for this object (one per frame in flight)
        std::vector<VkDescriptorSet> descriptor_sets;

        // Calculate model matrix based on position, rotation, and scale
        glm::mat4 getModelMatrix() const {
            glm::mat4 model = glm::mat4(1.0F);
            model           = glm::translate(model, position);
            model           = glm::rotate(model, rotation.x, glm::vec3(1.0F, 0.0F, 0.0F));
            model           = glm::rotate(model, rotation.y, glm::vec3(0.0F, 1.0F, 0.0F));
            model           = glm::rotate(model, rotation.z, glm::vec3(0.0F, 0.0F, 1.0F));
            model           = glm::scale(model, scale);
            return model;
        }
    };

}; // namespace VKTest
