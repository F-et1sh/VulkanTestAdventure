#include "pch.h"
#include "VkHpp_Image.h"

VKHppTest::Image::Image(
    uint32_t                width,
    uint32_t                height,
    uint32_t                mip_levels,
    vk::SampleCountFlagBits num_samples,
    vk::Format              format,
    vk::ImageAspectFlags    aspect_flags,
    vk::ImageTiling         tiling,
    vk::ImageUsageFlags     usage,
    vk::MemoryPropertyFlags properties,
    vk::raii::Device&       device) {

    this->createImage(width, height, mip_levels, num_samples, format, tiling, usage, properties, m_Image, m_ImageMemory, device);
    m_ImageView = this->createImageView(m_Image, format, aspect_flags, mip_levels, device);
}

void VKHppTest::Image::createImage(
    uint32_t                width,
    uint32_t                height,
    uint32_t                mip_levels,
    vk::SampleCountFlagBits num_samples,
    vk::Format              format,
    vk::ImageTiling         tiling,
    vk::ImageUsageFlags     usage,
    vk::MemoryPropertyFlags properties,
    vk::raii::Image&        image,
    vk::raii::DeviceMemory& image_memory,
    vk::raii::Device&       device) {

    vk::ImageCreateInfo image_info{
        vk::ImageCreateFlags{}, // Flags
        vk::ImageType::e2D,     // Image Type
        format,                 // Image Format
        {
            // Image Extent
            width,  // Width
            height, // Height
            1       // Depth
        },
        mip_levels,                  // Mip Levels
        1,                           // Array Layers
        num_samples,                 // Number of Samples
        tiling,                      // Image Tiling
        usage,                       // Image Usage
        vk::SharingMode::eExclusive, // Sharing Mode
        {},                          // Queue Family Index Count
        {},                          // Queue Family Indices
        vk::ImageLayout::eUndefined  // Initial Image Layout
    };

    image = vk::raii::Image{ device, image_info };

    vk::MemoryRequirements mem_requirements = image.getMemoryRequirements();

    vk::MemoryAllocateInfo alloc_info{
        mem_requirements.size,          // Allocation Size
        mem_requirements.memoryTypeBits // Memory Type Index
    };

    image_memory = vk::raii::DeviceMemory{ device, alloc_info };

    image.bindMemory(image_memory, 0);
}

void VKHppTest::Image::Initialize(
    uint32_t                width,
    uint32_t                height,
    uint32_t                mip_levels,
    vk::SampleCountFlagBits num_samples,
    vk::Format              format,
    vk::ImageAspectFlags    aspect_flags,
    vk::ImageTiling         tiling,
    vk::ImageUsageFlags     usage,
    vk::MemoryPropertyFlags properties,
    vk::raii::Device&       device) {

    this->createImage(width, height, mip_levels, num_samples, format, tiling, usage, properties, m_Image, m_ImageMemory, device);
    m_ImageView = this->createImageView(m_Image, format, aspect_flags, mip_levels, device);
}

vk::raii::ImageView VKHppTest::Image::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags, uint32_t mip_levels, vk::raii::Device& device) {
    vk::ImageViewCreateInfo create_info{
        vk::ImageViewCreateFlags{}, // Flags
        image,                      // Image
        vk::ImageViewType::e2D,     // Image View Type
        format,                     // Format
        {},                         // Components
        vk::ImageSubresourceRange{
            aspect_flags, // Aspect Mask
            0,            // Base Mip Level
            mip_levels,   // Level Count
            0,            // Base Array Layer
            1,            // Layer Count
        }
    };

    return vk::raii::ImageView{ device, create_info };
}
