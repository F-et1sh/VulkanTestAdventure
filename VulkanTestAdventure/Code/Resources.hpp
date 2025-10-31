#pragma once

namespace vk_test {
    // Forward declare VmaAllocation so we don't need to pull in vk_mem_alloc.h
    VK_DEFINE_HANDLE(VmaAllocation)

    //-----------------------------------------------------------------
    // A buffer is a region of memory used to store data.
    // It is used to store vertex data, index data, uniform data, and other types of data.
    // There is a VkBuffer object that represents the buffer, and a VmaAllocation object that represents the memory allocation.
    // The address is used to access the buffer in the shader.
    //
    struct Buffer {
        VkBuffer        buffer{}; // Vulkan Buffer
        VkDeviceSize    bufferSize{};
        VkDeviceAddress address{}; // Address of the buffer in the shader
        uint8_t*        mapping{};
        VmaAllocation   allocation{}; // Memory associated with the buffer
    };

    //-----------------------------------------------------------------
    // An acceleration structure is a region of memory used to store acceleration structure data.
    // It is used to store acceleration structure data for ray tracing.
    //-----------------------------------------------------------------
    struct AccelerationStructure {
        VkAccelerationStructureKHR accel{};
        VkDeviceAddress            address{};
        Buffer                     buffer; // Underlying buffer
    };

    // allows >= 4GB sizes
    struct LargeAccelerationStructure {
        VkAccelerationStructureKHR accel{};
        VkDeviceAddress            address{};
        LargeBuffer                buffer{}; // Underlying buffer
    };

    // Allows to represent >= 4 GB buffers using sparse bindings
    struct LargeBuffer {
        VkBuffer                   buffer{};
        VkDeviceSize               bufferSize{};
        VkDeviceAddress            address{};
        std::vector<VmaAllocation> allocations;
    };

    //-----------------------------------------------------------------
    // An image is a region of memory used to store image data.
    // It is used to store texture data, framebuffer data, and other types of data.
    //-----------------------------------------------------------------
    struct Image {
        // Vulkan Image
        // created/destroyed by `nvvk::ResourceAllocator`
        VkImage image{};
        // Size of the image
        VkExtent3D extent{};
        // Number of mip levels
        uint32_t mipLevels{};
        // number of array layers
        uint32_t arrayLayers{};
        // format of the image itself
        VkFormat format{ VK_FORMAT_UNDEFINED };
        // Memory associated with the image
        // managed by `nvvk::ResourceAllocator`
        VmaAllocation allocation{};

        // descriptor.imageLayout represents the current imageLayout
        // descriptor.imageView may exist, created/destroyed by `nvvk::ResourceAllocator`
        // descriptor.sampler may exist, not managed by `nvvk::ResourceAllocator`
        VkDescriptorImageInfo descriptor{};
    };

    struct QueueInfo {
        uint32_t family_index = ~0U; // Family index of the queue (graphic, compute, transfer, ...)
        uint32_t queue_index  = ~0U; // Index of the queue in the family
        VkQueue  queue{};            // The queue object
    };

    struct SemaphoreInfo {
        VkSemaphore semaphore{}; // Timeline semaphore
        uint64_t    value{};     // Timeline value
    };

} // namespace vk_test
