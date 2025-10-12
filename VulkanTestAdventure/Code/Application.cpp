#include "pch.h"
#include "Application.h"

VKTest::Application::~Application() {
}

void VKTest::Application::Loop() {
    m_Renderer.Initialize();

    while (!glfwWindowShouldClose(m_Window.getGLFWWindow())) {

        m_Renderer.DrawFrame();

        m_Window.PollEvents();
    }
}
