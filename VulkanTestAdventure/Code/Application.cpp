#include "pch.h"
#include "Application.h"

VKTest::Application::~Application() {

}

void VKTest::Application::Loop() {
	while (m_Window.isRunning()) {
		m_Window.PollEvents();
	}
}