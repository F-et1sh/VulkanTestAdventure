#include "pch.h"
#include "Application.h"

vk_test::Application::~Application() {
}

void vk_test::Application::Loop() {
    while (m_Window.isRunning()) {

        m_Renderer.DrawFrame();

        m_Window.PollEvents();
    }
}
