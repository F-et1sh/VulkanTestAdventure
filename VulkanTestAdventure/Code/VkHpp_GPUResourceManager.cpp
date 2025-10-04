#include "pch.h"
#include "VkHpp_GPUResourceManager.h"

#include "VkHpp_SwapchainManager.h"

VKHppTest::GPUResourceManager::GPUResourceManager(DeviceManager* device_manager, SwapchainManager* swapchain_manager, RenderPassManager* render_pass_manager) :
    p_DeviceManager{ device_manager }, p_SwapchainManager{ swapchain_manager }, p_RenderPassManager{ render_pass_manager } {}

void VKHppTest::GPUResourceManager::CreateDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding ubo_layout_binding{
        0,                                          // Binding
        vk::DescriptorType::eUniformBuffer,         // Descriptor Type
        1,                                          // Descriptor Count
        vk::ShaderStageFlagBits::eVertex,           // Stage Flags
        nullptr                                     // Immutable Samplers
    };

    vk::DescriptorSetLayoutBinding sampler_layout_binding{
        1,                                          // Binding
        vk::DescriptorType::eCombinedImageSampler,  // Descriptor Type
        1,                                          // Descriptor Count
        vk::ShaderStageFlagBits::eFragment,         // Stage Flags
        nullptr                                     // Immutable Samplers
    };

    std::array<vk::DescriptorSetLayoutBinding, 2> bindings = { ubo_layout_binding, sampler_layout_binding };
    vk::DescriptorSetLayoutCreateInfo layout_info{
        vk::DescriptorSetLayoutCreateFlags{},   // Flags
        static_cast<uint32_t>(bindings.size()), // Binding Count
        bindings.data()                         // Bindings
    };

    auto& device = p_DeviceManager->getDevice();
    m_DescriptorSetLayout = device.createDescriptorSetLayout(layout_info);
}

void VKHppTest::GPUResourceManager::CreateColorResources() {
    vk::Format color_format = this->p_SwapchainManager->getFormat();
    vk::Extent2D extent = this->p_SwapchainManager->getExtent();
    constexpr static uint32_t mip_levels = 1;

    auto& device = p_DeviceManager->getDevice();
    m_ColorImage.Initialize(extent.width, extent.height, mip_levels, p_RenderPassManager->getMSAASamples(), color_format, vk::ImageAspectFlagBits::eColor, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, device);
}

void VKHppTest::GPUResourceManager::CreateDepthResources() {
    vk::Format depth_format = this->p_DeviceManager->findDepthFormat();
    vk::Extent2D extent = this->p_SwapchainManager->getExtent();
    constexpr static uint32_t mip_levels = 1;

    auto& device = p_DeviceManager->getDevice();
    m_DepthImage.Initialize(extent.width, extent.height, mip_levels, p_RenderPassManager->getMSAASamples(), depth_format, vk::ImageAspectFlagBits::eDepth, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, device);
}

void VKHppTest::GPUResourceManager::CreateDescriptorPool() {
    // we need MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT descriptor sets
    std::array pool_size{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT)
    };
    vk::DescriptorPoolCreateInfo pool_info{
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, // Flags
        MAX_OBJECTS * MAX_FRAMES_IN_FLIGHT,                   // Max Sets
        static_cast<uint32_t>(pool_size.size()),              // Pool Size Count
        pool_size.data()                                      // Pool Sizes
    };

    auto& device = p_DeviceManager->getDevice();
    m_DescriptorPool = device.createDescriptorPool(pool_info);
}

void VKHppTest::GPUResourceManager::CreateDescriptorSets() {
    // for each game object
    for (auto& game_object : m_GameObjects) {
        // create descriptor sets for each frame in flight
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_DescriptorSetLayout);

        vk::DescriptorSetAllocateInfo alloc_info{
            m_DescriptorPool,                      // Descriptor Pool
            static_cast<uint32_t>(layouts.size()), // Set Layout Count
            layouts.data(),                        // Set Layouts
        };

        auto& device = p_DeviceManager->getDevice();
        game_object.descriptor_sets = device.allocateDescriptorSets(alloc_info);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo buffer_info{
                game_object.uniform_buffers[i],  // Buffer
                0,                              // Offset
                sizeof(UniformBufferObject)     // Range
            };

            vk::DescriptorImageInfo image_info{
                *m_TextureSampler,                       // Sampler
                *m_TextureImage.getImageView(),          // Image View
                vk::ImageLayout::eShaderReadOnlyOptimal // Image Layout
            };

            std::array descriptor_writes{
                vk::WriteDescriptorSet{
                    game_object.descriptor_sets[i],     // dst Set
                    0,                                  // dst Binding
                    0,                                  // dst Array Element
                    1,                                  // Descriptor Count
                    vk::DescriptorType::eUniformBuffer, // Descriptor Type
                    {},                                 // Image Info
                    &buffer_info                        // Buffer Info
                },
                vk::WriteDescriptorSet{
                    game_object.descriptor_sets[i],     // dst Set
                    1,                                  // dst Binding
                    0,                                  // dst Array Element
                    1,                                  // Descriptor Count
                    vk::DescriptorType::eSampledImage,  // Descriptor Type
                    &image_info,                        // Image Info
                    {},                                 // Buffer Info
                }
            };

            device.updateDescriptorSets(descriptor_writes, {});
        }
    }
}

void VKHppTest::GPUResourceManager::CreateVertexBuffer() {
    vk::DeviceSize buffer_size = sizeof(VERTICES[0]) * VERTICES.size();

    vk::raii::Buffer staging_buffer = VK_NULL_HANDLE;
    vk::raii::DeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    this->createBuffer(buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, staging_buffer, staging_buffer_memory);

    auto& device = p_DeviceManager->getDevice();

    void* data = staging_buffer_memory.mapMemory(0, buffer_size);
    memcpy(data, VERTICES.data(), (size_t)buffer_size);
    staging_buffer_memory.unmapMemory();

    createBuffer(buffer_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_Vertices, m_VerticesMemory);

    copyBuffer(staging_buffer, m_Vertices, buffer_size);
};

void VKHppTest::GPUResourceManager::CreateIndexBuffer() {
    vk::DeviceSize buffer_size = sizeof(INDICES[0]) * INDICES.size();

    vk::raii::Buffer staging_buffer = VK_NULL_HANDLE;
    vk::raii::DeviceMemory staging_buffer_memory = VK_NULL_HANDLE;
    this->createBuffer(buffer_size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, staging_buffer, staging_buffer_memory);

    auto& device = p_DeviceManager->getDevice();

    void* data = staging_buffer_memory.mapMemory(0, buffer_size);
    memcpy(data, INDICES.data(), (size_t)buffer_size);
    staging_buffer_memory.unmapMemory();

    createBuffer(buffer_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, m_Indices, m_IndicesMemory);

    copyBuffer(staging_buffer, m_Indices, buffer_size);
}

void VKHppTest::GPUResourceManager::CreateUniformBuffers() {
    // For each game object
    for (auto& game_object : m_GameObjects) {
        vk::DeviceSize buffer_size = sizeof(UniformBufferObject);

        game_object.uniform_buffers.reserve(MAX_FRAMES_IN_FLIGHT);
        game_object.uniform_buffers_memory.reserve(MAX_FRAMES_IN_FLIGHT);
        game_object.uniform_buffers_mapped.reserve(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::raii::Buffer buffer = VK_NULL_HANDLE;
            vk::raii::DeviceMemory buffer_memory = VK_NULL_HANDLE;

            createBuffer(buffer_size, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, buffer_memory);
            game_object.uniform_buffers.emplace_back(std::move(buffer));
            game_object.uniform_buffers_memory.emplace_back(std::move(buffer_memory));

            game_object.uniform_buffers_memory[i].mapMemory(0, buffer_size);
        }
    };
}

void VKHppTest::GPUResourceManager::CreateSyncObjects() {
    m_ImageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semaphore_info{};

    vk::FenceCreateInfo fence_info{
        vk::FenceCreateFlagBits::eSignaled // Flags
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        auto& device = p_DeviceManager->getDevice();

        m_ImageAvailableSemaphores.emplace_back(device.createSemaphore(semaphore_info));
        m_RenderFinishedSemaphores.emplace_back(device.createSemaphore(semaphore_info));
        m_InFlightFences.emplace_back(device.createFence(fence_info));
    }
}

void VKHppTest::GPUResourceManager::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& buffer_memory)const {
    vk::BufferCreateInfo buffer_info{
        vk::BufferCreateFlags{},    // Flags
        size,                       // Size
        usage,                      // Usage
        vk::SharingMode::eExclusive // Sharing Mode
    };

    auto& device = p_DeviceManager->getDevice();

    buffer = vk::raii::Buffer{ device, buffer_info };
    vk::MemoryRequirements memory_requirements = buffer.getMemoryRequirements();
    
    vk::MemoryAllocateInfo alloc_info{
        memory_requirements.size, // Allocation Size
        p_DeviceManager->findMemoryType(memory_requirements.memoryTypeBits, properties)
    };

    buffer_memory = vk::raii::DeviceMemory{ device, alloc_info };

    buffer.bindMemory(buffer_memory, 0);
}

void VKHppTest::GPUResourceManager::copyBuffer(vk::raii::Buffer& src_buffer, vk::raii::Buffer& dst_buffer, vk::DeviceSize size)const {
    vk::raii::CommandBuffer command_buffer = p_DeviceManager->beginSingleTimeCommands();

    vk::BufferCopy copy_region{
        0,   // Src Offset
        0,   // Dst Offset
        size // Size
    };
    command_buffer.copyBuffer(src_buffer, dst_buffer, copy_region);

    p_DeviceManager->endSingleTimeCommands(command_buffer);
}