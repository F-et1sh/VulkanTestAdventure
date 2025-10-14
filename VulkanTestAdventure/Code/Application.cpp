#include "pch.h"
#include "Application.h"

VKTest::Application::~Application() {
}

void VKTest::Application::Initialize() {
    m_Renderer.Initialize();
}

void VKTest::Application::Loop() {
    while (glfwWindowShouldClose(m_Window.getGLFWWindow()) == 0) {

        m_Renderer.DrawFrame();

        VKTest::Window::PollEvents();
    }
}
