#pragma once
#include "Window.hpp"
#include "Context.hpp"

namespace vk_test {
    class Application {
    public:
        Application(const glm::vec2& window_resolution, const std::string& window_title, int window_monitor)
            : m_Window{ window_resolution, window_title, window_monitor } {}
        ~Application();

        void Initialize();
        void Loop();

    private:
        Window  m_Window;
        Context m_Context;
    };
} // namespace vk_test
