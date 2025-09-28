#include "pch.h"
#include "GPUResourceManager.h"

VKTest::GPUResourceManager::GPUResourceManager(DeviceManager* device_manager, SwapchainManager* swapchain_manager, RenderPassManager* render_pass_manager) :
    p_DeviceManager{ device_manager }, p_SwapchainManager{ swapchain_manager }, p_RenderPassManager{ render_pass_manager } {}

void VKTest::GPUResourceManager::CreateDescriptorSetLayout() {
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

void VKTest::GPUResourceManager::CreateColorResources() {
    vk::Format color_format = this->p_SwapchainManager->getFormat();
    vk::Extent2D extent = this->p_SwapchainManager->getExtent();
    constexpr static uint32_t mip_levels = 1;

    auto& device = p_DeviceManager->getDevice();
    m_ColorImage.Initialize(extent.width, extent.height, mip_levels, p_RenderPassManager->getMSAASamples(), color_format, vk::ImageAspectFlagBits::eColor, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, device);
}

void VKTest::GPUResourceManager::CreateDepthResources() {
    vk::Format depth_format = this->p_DeviceManager->findDepthFormat();
    vk::Extent2D extent = this->p_SwapchainManager->getExtent();
    constexpr static uint32_t mip_levels = 1;

    auto& device = p_DeviceManager->getDevice();
    m_DepthImage.Initialize(extent.width, extent.height, mip_levels, p_RenderPassManager->getMSAASamples(), depth_format, vk::ImageAspectFlagBits::eDepth, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, device);
}

void VKTest::GPUResourceManager::CreateDescriptorPool() {
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

void VKTest::GPUResourceManager::CreateDescriptorSets() {
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
                m_TextureSampler,                       // Sampler
                m_TextureImage.getImageView(),          // Image View
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