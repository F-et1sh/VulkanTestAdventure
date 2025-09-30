#include "pch.h"
#include "Application.h"

VKTest::Application::~Application() {

}

void VKTest::Application::Loop() {
	while (m_Window.isRunning()) {

		m_Renderer.DrawFrame();

		m_Window.PollEvents();
	}
}