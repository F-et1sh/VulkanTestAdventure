#pragma once

namespace VKTest {
	class Renderer; // forward declaration

	class SwapchainManager {
	public:
		SwapchainManager(Renderer* renderer) : p_Renderer{ renderer } {};
		~SwapchainManager() = default;

	private:
		Renderer* p_Renderer = nullptr;
	};
}