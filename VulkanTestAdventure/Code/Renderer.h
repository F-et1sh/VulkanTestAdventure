#pragma once
#include "Window.h"
#include "DeviceManager.h"

namespace vk_test {
    class Renderer {
    public:
        Renderer(Window* window) : p_Window{ window } {}
        ~Renderer() = default;

        void DrawFrame();

        inline Window* getWindow() noexcept { return p_Window; }

    private:
        Window* p_Window = nullptr;

        DeviceManager m_DeviceManager;
    };
} // namespace vk_test
