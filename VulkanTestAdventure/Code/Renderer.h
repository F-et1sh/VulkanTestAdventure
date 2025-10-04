#pragma once
#include "Window.h"

namespace VKTest {
	class Renderer {
	public:
		Renderer(Window* window) : p_Window{ window } { }
		~Renderer() = default;

		void DrawFrame();

		inline Window* getWindow()noexcept { return p_Window; }

	private:
		Window* p_Window = nullptr;
	};
}