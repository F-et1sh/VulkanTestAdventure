#pragma once
#include "Window.h"
#include "VKTest_DeviceManager.h"
#include "VKTest_SwapchainManager.h"
#include "VKTest_RenderPassManager.h"
#include "VKTest_PipelineManager.h"

namespace VKTest {
    class Renderer {
    public:
        Renderer(Window* window) : p_Window{ window },
                                   m_DeviceManager{ &m_SwapchainManager, window, &m_RenderPassManager, &m_PipelineManager, &m_RenderMesh },
                                   m_SwapchainManager{ &m_DeviceManager, p_Window, &m_RenderPassManager },
                                   m_RenderPassManager{ &m_DeviceManager, &m_SwapchainManager },
                                   m_PipelineManager{ &m_DeviceManager, &m_RenderPassManager, &m_SwapchainManager, &m_RenderMesh } {}
        ~Renderer();

        void Initialize();

        void DrawFrame();

        Window* getWindow() noexcept { return p_Window; }

    private:
        Window* p_Window = nullptr;

        DeviceManager     m_DeviceManager;
        SwapchainManager  m_SwapchainManager;
        RenderPassManager m_RenderPassManager;
        PipelineManager   m_PipelineManager;

        RenderMesh m_RenderMesh;
    };
} // namespace VKTest
