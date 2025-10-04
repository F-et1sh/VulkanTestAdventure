#pragma once

namespace VKTest {
	class DeciceManager {
	public:
		DeciceManager() = default;
		~DeciceManager() = default;

	private:
		VkInstance m_Instance;

		VkPhysicalDevice m_PhysicalDevice;
		VkDevice m_Device;

		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;

		VkDebugUtilsMessengerEXT m_DebugMessenger;
	};
}