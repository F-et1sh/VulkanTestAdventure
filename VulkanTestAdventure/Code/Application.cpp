#include "pch.h"
#include "Application.h"

VKHppTest::Application::~Application() {

}

void VKHppTest::Application::Loop() {
	while (m_Window.isRunning()) {

		m_Renderer.DrawFrame();

		m_Window.PollEvents();
	}
}