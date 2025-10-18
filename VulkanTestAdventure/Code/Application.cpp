#include "pch.h"
#include "Application.hpp"

vk_test::Application::~Application() {
}

void vk_test::Application::Initialize() {
}

void vk_test::Application::Loop() {
    while (glfwWindowShouldClose(m_Window.getGLFWWindow()) == 0) {

        vk_test::Window::PollEvents();
    }
}
