#include "pch.h"
#include "RenderMesh.h"

// NOLINTNEXTLINE(readability-identifier-naming)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// NOLINTNEXTLINE(readability-identifier-naming)
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

void VKTest::RenderMesh::Initialize(DeviceManager* device_manager) {
    p_DeviceManager = device_manager;

    this->createTextureImage();
    this->createTextureImageView();
    this->createTextureSampler();

    this->loadModel();

    this->createVertexBuffer();
    this->createIndexBuffer();

    this->createUniformBuffers();
}

void VKTest::RenderMesh::createTextureImage() {
    int          texture_width    = 0;
    int          texture_height   = 0;
    int          texture_channels = 0;
    stbi_uc*     pixels           = stbi_load(TEXTURE_PATH.c_str(), &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
    VkDeviceSize image_size       = texture_width * texture_height * 4;
    m_MipLevels                   = static_cast<uint32_t>(std::floor(std::log2(std::max(texture_width, texture_height)))) + 1;

    if (pixels == nullptr) {
        VK_TEST_RUNTIME_ERROR(std::string("ERROR : Failed to load texture image\nPath : ") + TEXTURE_PATH);
    }

    VkBuffer       staging_buffer        = nullptr;
    VkDeviceMemory staging_buffer_memory = nullptr;
    p_DeviceManager->createBuffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

    void* data = nullptr;
    vkMapMemory(p_DeviceManager->getDevice(), staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(image_size));
    vkUnmapMemory(p_DeviceManager->getDevice(), staging_buffer_memory);

    stbi_image_free(pixels);

    p_DeviceManager->createImage(
        texture_width,
        texture_height,
        m_MipLevels,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_TextureImage,
        m_TextureImageMemory);

    p_DeviceManager->transitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
    p_DeviceManager->copyBufferToImage(staging_buffer, m_TextureImage, static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height));
    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

    vkDestroyBuffer(p_DeviceManager->getDevice(), staging_buffer, nullptr);
    vkFreeMemory(p_DeviceManager->getDevice(), staging_buffer_memory, nullptr);

    generateMipmaps(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, texture_width, texture_height, m_MipLevels);
}

void VKTest::RenderMesh::generateMipmaps(VkImage image, VkFormat image_format, int32_t texture_width, int32_t texture_height, uint32_t mip_levels) {
    // Check if image format supports linear blitting
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(p_DeviceManager->getPhysicalDevice(), image_format, &format_properties);

    if ((format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0U) {
        VK_TEST_RUNTIME_ERROR("ERROR : Texture image format does not support linear blitting");
    }

    VkCommandBuffer command_buffer = p_DeviceManager->beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image                           = image;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;
    barrier.subresourceRange.levelCount     = 1;

    int32_t mip_width  = texture_width;
    int32_t mip_height = texture_height;

    for (uint32_t i = 1; i < mip_levels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0]                 = { 0, 0, 0 };
        blit.srcOffsets[1]                 = { mip_width, mip_height, 1 };
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = 1;
        blit.dstOffsets[0]                 = { 0, 0, 0 };
        blit.dstOffsets[1]                 = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 };
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = 1;

        vkCmdBlitImage(command_buffer,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &blit,
                       VK_FILTER_LINEAR);

        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(command_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        if (mip_width > 1) {
            mip_width /= 2;
        }
        if (mip_height > 1) {
            mip_height /= 2;
        }
    }

    barrier.subresourceRange.baseMipLevel = mip_levels - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    p_DeviceManager->endSingleTimeCommands(command_buffer);
}

void VKTest::RenderMesh::createTextureImageView() {
    m_TextureImageView = p_DeviceManager->createImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);
}

void VKTest::RenderMesh::createTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(p_DeviceManager->getPhysicalDevice(), &properties);

    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter               = VK_FILTER_LINEAR;
    sampler_info.minFilter               = VK_FILTER_LINEAR;
    sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable        = VK_TRUE;
    sampler_info.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
    sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable           = VK_FALSE;
    sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.minLod                  = 0.0F;
    sampler_info.maxLod                  = VK_LOD_CLAMP_NONE;
    sampler_info.mipLodBias              = 0.0F;

    if (vkCreateSampler(p_DeviceManager->getDevice(), &sampler_info, nullptr, &m_TextureSampler) != VK_SUCCESS) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to create texture sampler");
    }
}

void VKTest::RenderMesh::loadModel() {
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str())) {
        throw std::runtime_error(err);
    }

    std::unordered_map<Vertex, uint32_t> unique_vertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = {
                attrib.vertices[(3 * index.vertex_index) + 0],
                attrib.vertices[(3 * index.vertex_index) + 1],
                attrib.vertices[(3 * index.vertex_index) + 2]
            };

            vertex.texture_coord = {
                attrib.texcoords[(2 * index.texcoord_index) + 0],
                1.0F - attrib.texcoords[(2 * index.texcoord_index) + 1]
            };

            vertex.color = { 1.0F, 1.0F, 1.0F };

            if (!unique_vertices.contains(vertex)) {
                unique_vertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
                m_Vertices.push_back(vertex);
            }

            m_Indices.push_back(unique_vertices[vertex]);
        }
    }
}

void VKTest::RenderMesh::createVertexBuffer() {
    VkDeviceSize buffer_size = sizeof(m_Vertices[0]) * m_Vertices.size();

    VkBuffer       staging_buffer        = nullptr;
    VkDeviceMemory staging_buffer_memory = nullptr;
    p_DeviceManager->createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

    void* data = nullptr;
    vkMapMemory(p_DeviceManager->getDevice(), staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, m_Vertices.data(), (size_t) buffer_size);
    vkUnmapMemory(p_DeviceManager->getDevice(), staging_buffer_memory);

    p_DeviceManager->createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

    p_DeviceManager->copyBuffer(staging_buffer, m_VertexBuffer, buffer_size);

    vkDestroyBuffer(p_DeviceManager->getDevice(), staging_buffer, nullptr);
    vkFreeMemory(p_DeviceManager->getDevice(), staging_buffer_memory, nullptr);
}

void VKTest::RenderMesh::createIndexBuffer() {
    VkDeviceSize buffer_size = sizeof(m_Indices[0]) * m_Indices.size();

    VkBuffer       staging_buffer        = nullptr;
    VkDeviceMemory staging_buffer_memory = nullptr;
    p_DeviceManager->createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

    void* data = nullptr;
    vkMapMemory(p_DeviceManager->getDevice(), staging_buffer_memory, 0, buffer_size, 0, &data);
    memcpy(data, m_Indices.data(), (size_t) buffer_size);
    vkUnmapMemory(p_DeviceManager->getDevice(), staging_buffer_memory);

    p_DeviceManager->createBuffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

    p_DeviceManager->copyBuffer(staging_buffer, m_IndexBuffer, buffer_size);

    vkDestroyBuffer(p_DeviceManager->getDevice(), staging_buffer, nullptr);
    vkFreeMemory(p_DeviceManager->getDevice(), staging_buffer_memory, nullptr);
}

void VKTest::RenderMesh::createUniformBuffers() {
    // For each game object
    for (auto& game_object : GAME_OBJECTS) {
        VkDeviceSize buffer_size = sizeof(UniformBufferObject);

        game_object.uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
        game_object.uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
        game_object.uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            p_DeviceManager->createBuffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, game_object.uniform_buffers[i], game_object.uniform_buffers_memory[i]);

            vkMapMemory(p_DeviceManager->getDevice(), game_object.uniform_buffers_memory[i], 0, buffer_size, 0, &game_object.uniform_buffers_mapped[i]);
        }
    }
}
