#pragma once
#include "Window.h"
#include "Renderer.h"

namespace VKTest {
	class Application {
	public:
		Application() = default;
		~Application();

		void Initialize(const glm::vec2& window_resolution, const std::string& window_title, int window_monitor);
		void Loop();

	private:
		Window m_Window;
		Renderer m_Renderer{ &m_Window };
	};
}