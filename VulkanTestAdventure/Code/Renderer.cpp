#include "pch.h"
#include "Renderer.h"

void VKTest::Renderer::Release() {
}

void VKTest::Renderer::Initialize() {
    m_DeviceManager.CreateInstance();
    m_DeviceManager.SetupDebugMessenger();

    m_SwapchainManager.CreateSurface();

    m_DeviceManager.PickPhysicalDevice();

    m_DeviceManager.CreateLogicalDevice();
    m_SwapchainManager.CreateSwapchain();
}

void VKTest::Renderer::DrawFrame() {
}
