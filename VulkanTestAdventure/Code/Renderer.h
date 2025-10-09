#pragma once
#include "Window.h"
#include "DeviceManager.h"

namespace VKTest {
    class Renderer {
    public:
        Renderer(Window* window) : p_Window{ window } {}
        ~Renderer() = default;

        void DrawFrame();

        Window* getWindow() noexcept { return p_Window; }

    private:
        Window* p_Window = nullptr;

        DeviceManager m_DeviceManager;
    };
} // namespace VKTest
