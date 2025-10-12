#pragma once
#include "GameObjects.h"

namespace VKTest {
    class DeviceManager; // forward declaration

    const std::string MODEL_PATH   = "F:/Windows/Desktop/VulkanTestAdventure/Files/Models/viking_room.obj";
    const std::string TEXTURE_PATH = "F:/Windows/Desktop/VulkanTestAdventure/Files/Textures/viking_room.png";

    class RenderMesh {
    public:
        RenderMesh()  = default;
        ~RenderMesh() = default;

        void Initialize(DeviceManager* device_manager);

        VkSampler   getTextureSampler() const noexcept { return m_TextureSampler; }
        VkImageView getTextureImageView() const noexcept { return m_TextureImageView; }

    private:
        void createTextureImage();
        void createTextureImageView();
        void createTextureSampler();
        void loadModel();
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffers();

        void generateMipmaps(VkImage image, VkFormat image_format, int32_t texture_width, int32_t texture_height, uint32_t mip_levels);

    private:
        DeviceManager* p_DeviceManager = nullptr;

        uint32_t       m_MipLevels = 0;
        VkImage        m_TextureImage{};
        VkDeviceMemory m_TextureImageMemory{};
        VkImageView    m_TextureImageView{};
        VkSampler      m_TextureSampler{};

        std::vector<Vertex>   m_Vertices;
        std::vector<uint32_t> m_Indices;

        VkBuffer       m_VertexBuffer{};
        VkDeviceMemory m_VertexBufferMemory{};

        VkBuffer       m_IndexBuffer{};
        VkDeviceMemory m_IndexBufferMemory{};
    };
} // namespace VKTest
