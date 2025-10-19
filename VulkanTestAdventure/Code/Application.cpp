#include "pch.h"
#include "Application.hpp"

vk_test::Application::~Application() {
}

void vk_test::Application::Initialize() {
    m_Renderer.Initialize();
}

void vk_test::Application::Loop() {
    while (glfwWindowShouldClose(m_Window.getGLFWWindow()) == 0) {
        m_Renderer.DrawFrame();
        vk_test::Window::PollEvents();
    }
}
