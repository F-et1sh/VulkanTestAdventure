#include "pch.h"
#include "Application.h"

VKTest::Application::~Application() {
}

void VKTest::Application::Loop() {
    m_Renderer.Initialize();

    while (m_Window.isRunning()) {

        m_Renderer.DrawFrame();

        VKTest::Window::PollEvents();
    }
}
