#include "pch.h"
#include "GPUResourceManager.h"

VKTest::GPUResourceManager::GPUResourceManager(DeviceManager* device_manager, SwapchainManager* swapchain_manager) : p_DeviceManager{ device_manager }, p_SwapchainManager{ swapchain_manager } {}

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
    m_ColorImage.Initialize(extent.width, extent.height, mip_levels, m_MSAA_Samples, color_format, vk::ImageAspectFlagBits::eColor, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, device);
}

void VKTest::GPUResourceManager::CreateDepthResources() {
    vk::Format depth_format = m_DeviceManager.findDepthFormat();
    vk::Extent2D extent = this->m_SwapchainManager.getExtent();
    constexpr static uint32_t mip_levels = 1;

    m_DeviceManager.createImage(extent.width, extent.height, mip_levels, m_MSAA_Samples, depth_format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, m_DepthImage, m_DepthImageMemory);
    auto image_view = m_DeviceManager.createImageView(m_DepthImage, depth_format, vk::ImageAspectFlagBits::eDepth, mip_levels);

    m_DepthImageView = std::move(image_view);
}
