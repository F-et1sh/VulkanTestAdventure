#pragma once
#include "Window.h"
#include "DeviceManager.h"
#include "SwapchainManager.h"

namespace VKTest {
    class Renderer {
    public:
        Renderer(Window* window) : p_Window{ window }, m_DeviceManager{ &m_SwapchainManager }, m_SwapchainManager{ &m_DeviceManager, p_Window } {}
        ~Renderer() { this->Release(); }

        void Release();
        void Initialize();

        void DrawFrame();

        Window* getWindow() noexcept { return p_Window; }

    private:
        Window* p_Window = nullptr;

        DeviceManager    m_DeviceManager;
        SwapchainManager m_SwapchainManager;
    };
} // namespace VKTest
