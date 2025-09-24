#pragma once

namespace VKTest {
	class Window {
	public:
		Window();
		~Window();

		void CreateWindow(const glm::vec2& window_resolution, const std::string& window_title, int window_monitor);

		void ClearColor(glm::vec4 color)const noexcept;
		void ClearColor(float r, float g, float b, float a)const noexcept;
		
		void ClearScreen(unsigned int buffer_bit)const noexcept;
		
		void SwapBuffers()const noexcept;

		void PollEvents()const noexcept;

		inline constexpr bool isRunning()const noexcept { return m_IsRunning; }
		inline constexpr GLFWwindow* getGLFWwindow()const noexcept { return p_GLFWWindow; }

	private:
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
			auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
			app->m_FramebufferResized = true;
		}

	private:
		GLFWwindow* p_GLFWWindow = nullptr;
		bool m_FramebufferResized = false;

		bool m_IsRunning = false;
	};
}