#pragma once
#include "Window.h"
#include "Renderer.h"

namespace vk_test {
    class Application {
    public:
        Application(const glm::vec2& window_resolution, const std::string& window_title, int window_monitor) : m_Window{ window_resolution, window_title, window_monitor }, m_Renderer{ &m_Window } {}
        ~Application();

        void Loop();

    private:
        Window   m_Window;
        Renderer m_Renderer;
    };
} // namespace vk_test
