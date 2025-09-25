#include "pch.h"
#include "Application.h"

VKTest::Application::~Application() {

}

void VKTest::Application::Initialize(const glm::vec2& window_resolution, const std::string& window_title, int window_monitor) {
	m_Window.CreateWindow(window_resolution, window_title, window_monitor);


}

void VKTest::Application::Loop() {
	while (m_Window.isRunning()) {
		m_Window.PollEvents();
	}
}