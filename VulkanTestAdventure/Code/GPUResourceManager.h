#pragma once

namespace VKTest {
	class Renderer; // forward declaration

	class GPUResourceManager {
	public:
		GPUResourceManager(Renderer* renderer) : p_Renderer{ renderer } {};
		~GPUResourceManager() = default;

	private:
		Renderer* p_Renderer = nullptr;
	};
}