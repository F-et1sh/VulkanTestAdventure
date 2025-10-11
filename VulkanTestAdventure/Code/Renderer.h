#pragma once
#include "Window.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"
#include "RenderPassManager.h"

namespace VKTest {
    class Renderer {
    public:
        Renderer(Window* window)
            : p_Window{ window }, m_DeviceManager{ &m_SwapchainManager }, m_SwapchainManager{ &m_DeviceManager, p_Window }, m_RenderPassManager{ &m_DeviceManager, &m_SwapchainManager } {}
        ~Renderer() { this->Release(); }

        void Release();
        void Initialize();

        void DrawFrame();

        Window* getWindow() noexcept { return p_Window; }

    private:
        Window* p_Window = nullptr;

        DeviceManager     m_DeviceManager;
        SwapchainManager  m_SwapchainManager;
        RenderPassManager m_RenderPassManager;
    };
} // namespace VKTest
