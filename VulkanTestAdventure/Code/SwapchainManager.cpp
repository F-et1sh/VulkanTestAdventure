#include "pch.h"
#include "SwapchainManager.h"

void VKTest::SwapchainManager::Release() {
    vkDestroyImageView(p_DeviceManager->getDevice(), m_DepthImageView, nullptr);
    vkDestroyImage(p_DeviceManager->getDevice(), m_DepthImage, nullptr);
    vkFreeMemory(p_DeviceManager->getDevice(), m_DepthImageMemory, nullptr);

    vkDestroyImageView(p_DeviceManager->getDevice(), m_ColorImageView, nullptr);
    vkDestroyImage(p_DeviceManager->getDevice(), m_ColorImage, nullptr);
    vkFreeMemory(p_DeviceManager->getDevice(), m_ColorImageMemory, nullptr);

    for (auto* framebuffer : m_SwapchainFramebuffers) {
        vkDestroyFramebuffer(p_DeviceManager->getDevice(), framebuffer, nullptr);
    }

    for (auto* image_view : m_SwapchainImageViews) {
        vkDestroyImageView(p_DeviceManager->getDevice(), image_view, nullptr);
    }

    vkDestroySwapchainKHR(p_DeviceManager->getDevice(), m_Swapchain, nullptr);
}

void VKTest::SwapchainManager::CreateSurface() {
    if (glfwCreateWindowSurface(p_DeviceManager->getInstance(), p_Window->getGLFWWindow(), nullptr, &m_Surface) != VK_SUCCESS) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to create window surface");
    }
}

void VKTest::SwapchainManager::CreateSwapchain() {
    auto* physical_device = p_DeviceManager->getPhysicalDevice();
    auto* device          = p_DeviceManager->getDevice();
    auto* window          = p_Window->getGLFWWindow();

    SwapChainSupportDetails swap_chain_support = querySwapChainSupport(physical_device, m_Surface);

    VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR   present_mode   = chooseSwapPresentMode(swap_chain_support.present_modes);
    VkExtent2D         extent         = chooseSwapExtent(swap_chain_support.capabilities, window);

    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
    if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = m_Surface;

    create_info.minImageCount    = image_count;
    create_info.imageFormat      = surface_format.format;
    create_info.imageColorSpace  = surface_format.colorSpace;
    create_info.imageExtent      = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices                = p_DeviceManager->findQueueFamilies(p_DeviceManager->getPhysicalDevice(), m_Surface);
    uint32_t           queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices   = queue_family_indices;
    }
    else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    create_info.preTransform   = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode    = present_mode;
    create_info.clipped        = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &create_info, nullptr, &m_Swapchain) != VK_SUCCESS) {
        VK_TEST_RUNTIME_ERROR("ERROR : Failed to create swap chain");
    }

    vkGetSwapchainImagesKHR(device, m_Swapchain, &image_count, nullptr);
    m_SwapchainImages.resize(image_count);
    vkGetSwapchainImagesKHR(device, m_Swapchain, &image_count, m_SwapchainImages.data());

    m_SwapchainImageFormat = surface_format.format;
    m_SwapchainExtent      = extent;
}

void VKTest::SwapchainManager::CreateImageViews() {
    m_SwapchainImageViews.resize(m_SwapchainImages.size());

    for (uint32_t i = 0; i < m_SwapchainImages.size(); i++) {
        m_SwapchainImageViews[i] = p_DeviceManager->createImageView(m_SwapchainImages[i], m_SwapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

void VKTest::SwapchainManager::CreateColorResources() {
    VkFormat color_format = m_SwapchainImageFormat;

    p_DeviceManager->createImage(m_SwapchainExtent.width, m_SwapchainExtent.height, 1, p_DeviceManager->getMSAASamples(), color_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_ColorImage, m_ColorImageMemory);
    m_ColorImageView = p_DeviceManager->createImageView(m_ColorImage, color_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void VKTest::SwapchainManager::CreateDepthResources() {
    VkFormat depth_format = p_DeviceManager->findDepthFormat();

    p_DeviceManager->createImage(m_SwapchainExtent.width, m_SwapchainExtent.height, 1, p_DeviceManager->getMSAASamples(), depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
    m_DepthImageView = p_DeviceManager->createImageView(m_DepthImage, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void VKTest::SwapchainManager::CreateFramebuffers() {
    m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());

    for (size_t i = 0; i < m_SwapchainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            m_ColorImageView,
            m_DepthImageView,
            m_SwapchainImageViews[i]
        };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass      = p_RenderPassManager->getRenderPass();
        framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_info.pAttachments    = attachments.data();
        framebuffer_info.width           = m_SwapchainExtent.width;
        framebuffer_info.height          = m_SwapchainExtent.height;
        framebuffer_info.layers          = 1;

        if (vkCreateFramebuffer(p_DeviceManager->getDevice(), &framebuffer_info, nullptr, &m_SwapchainFramebuffers[i]) != VK_SUCCESS) {
            VK_TEST_RUNTIME_ERROR("ERROR : Failed to create framebuffer");
        }
    }
}

VkSurfaceFormatKHR VKTest::SwapchainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }

    return available_formats[0];
}

VkPresentModeKHR VKTest::SwapchainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VKTest::SwapchainManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width  = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actual_extent.width  = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actual_extent;
}

SwapChainSupportDetails VKTest::SwapchainManager::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapChainSupportDetails details{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.present_modes.data());
    }

    return details;
}

void VKTest::SwapchainManager::recreateSwapchain() {
    int width  = 0;
    int height = 0;
    glfwGetFramebufferSize(p_Window->getGLFWWindow(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(p_Window->getGLFWWindow(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(p_DeviceManager->getDevice());

    this->Release();

    this->CreateSwapchain();
    this->CreateImageViews();
    this->CreateColorResources();
    this->CreateDepthResources();
    this->CreateFramebuffers();
}
