#include "pch.h"
#include "Renderer.h"

void VKTest::Renderer::Release() {
}

void VKTest::Renderer::Initialize() {

    /*
        createSyncObjects();
    */

    m_DeviceManager.CreateInstance();
    m_DeviceManager.SetupDebugMessenger();

    m_SwapchainManager.CreateSurface();

    m_DeviceManager.PickPhysicalDevice();

    m_DeviceManager.CreateLogicalDevice();
    m_SwapchainManager.CreateSwapchain();

    m_RenderPassManager.CreateRenderPass();

    m_PipelineManager.CreateDescriptorSetLayout();
    m_PipelineManager.CreateGraphicsPipeline();

    m_DeviceManager.CreateCommandPool();

    m_SwapchainManager.CreateColorResources();
    m_SwapchainManager.CreateDepthResources();

    m_SwapchainManager.CreateFramebuffers();

    m_RenderMesh.Initialize(&m_DeviceManager);

    m_PipelineManager.CreateDescriptorPool();
    m_PipelineManager.CreateDescriptorSets();

    m_DeviceManager.CreateCommandBuffers();
}

void VKTest::Renderer::DrawFrame() {
}
