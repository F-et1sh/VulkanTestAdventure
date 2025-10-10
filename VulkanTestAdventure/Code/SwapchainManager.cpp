#include "pch.h"
#include "SwapchainManager.h"

void VKTest::SwapchainManager::Release() {
}

void VKTest::SwapchainManager::CreateSurface() {
    if (glfwCreateWindowSurface(p_DeviceManager->getInstance(), p_Window->getGLFWWindow(), nullptr, &m_Surface) != VK_SUCCESS) {
        VKTEST_RUNTIME_ERROR("ERROR : Failed to create window surface");
    }
}
