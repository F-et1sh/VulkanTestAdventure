#include "pch.h"
#include "SwapchainManager.h"

#include "Renderer.h"

VKTest::SwapchainManager::SwapchainManager(DeviceManager* p_device_manager, Window* p_window) : p_DeviceManager{ p_device_manager }, p_Window{ p_window } {
    
}

void VKTest::SwapchainManager::CreateSurface() {
    VkSurfaceKHR surface; // GLFW uses old C-Style Vulkan

    if (glfwCreateWindowSurface(*p_DeviceManager->getInstance(), p_Window->getGLFWWindow(), nullptr, &surface))
        RUNTIME_ERROR("Failed to create window surface");

    m_Surface = vk::raii::SurfaceKHR{ p_DeviceManager->getInstance(), surface };
}

void VKTest::SwapchainManager::CreateSwapchain() {
    auto& physical_device = p_DeviceManager->getPhysicalDevice();

    SwapChainSupportDetails swapchain_support = querySwapchainSupport(physical_device);

    vk::SurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swapchain_support.formats);
    vk::PresentModeKHR present_mode = chooseSwapPresentMode(swapchain_support.present_modes);
    vk::Extent2D extent = chooseSwapExtent(swapchain_support.capabilities);

    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;

    if (swapchain_support.capabilities.maxImageCount > 0 &&
        image_count > swapchain_support.capabilities.maxImageCount) {
        
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR create_info{
        vk::SwapchainCreateFlagsKHR{},              // Flags
        m_Surface,                                  // Surface
        image_count,                                // Minimal Image Count
        surface_format.format,                      // Image Format
        surface_format.colorSpace,                  // Image Color Space
        extent,                                     // Image Extent
        1,                                          // Image Array Layers
        vk::ImageUsageFlagBits::eColorAttachment    // Image Usage
    };

    //QueueFamilyIndices indices = p_DeviceManager->findQueueFamilies(physical_device);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VKTest::SwapchainManager::CreateImageViews() {
    m_SwapchainImageViews.reserve(m_SwapchainImages.size());

    for (uint32_t i = 0; i < m_SwapchainImages.size(); i++) {

        auto e = p_DeviceManager->CreateImageView(m_SwapchainImages[i], m_SwapchainImageFormat, vk::ImageAspectFlagBits::eColor, 1);
        m_SwapchainImageViews.emplace_back(std::move(e));
    }
}

SwapChainSupportDetails VKTest::SwapchainManager::querySwapchainSupport(vk::PhysicalDevice device) const {
    SwapChainSupportDetails details;

    details.capabilities = device.getSurfaceCapabilitiesKHR(m_Surface);
    details.formats = device.getSurfaceFormatsKHR(m_Surface);
    details.present_modes = device.getSurfacePresentModesKHR(m_Surface);

    return details;
}

vk::SurfaceFormatKHR VKTest::SwapchainManager::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats) const {
    for (const auto& available_format : available_formats) {

        if (available_format.format == vk::Format::eB8G8R8A8Srgb &&
            available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {

            return available_format;
        }
    }

    return available_formats[0];
}

vk::PresentModeKHR VKTest::SwapchainManager::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes) const {
    for (const auto& available_present_mode : available_present_modes)
        if (available_present_mode == vk::PresentModeKHR::eMailbox)
            return available_present_mode;
    return vk::PresentModeKHR::eFifo;
}

VkExtent2D VKTest::SwapchainManager::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) const {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else {
        int width = 0;
        int height = 0;
        GLFWwindow* glfw_window = p_Window->getGLFWWindow();
        glfwGetFramebufferSize(glfw_window, &width, &height);

        VkExtent2D actual_extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actual_extent;
    }
}
