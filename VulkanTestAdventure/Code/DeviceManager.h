#pragma once

namespace VKTest {
	class Renderer; // forward declaration

	class DeviceManager {
	private:
#ifdef _DEBUG
		constexpr static bool ENABLE_VALIDATION_LAYERS = true;
#else
		constexpr static bool ENABLE_VALIDATION_LAYERS = false;
#endif // _DEBUG

		constexpr static std::array VALIDATION_LAYERS = {
			"VK_LAYER_KHRONOS_validation"
		};

		constexpr static std::array REQUIRED_DEVICE_EXTENSION = {
			vk::KHRSwapchainExtensionName,
			vk::KHRSpirv14ExtensionName,
			vk::KHRSynchronization2ExtensionName,
			vk::KHRCreateRenderpass2ExtensionName
		};

	public:
		DeviceManager(Renderer* renderer) : p_Renderer{ renderer } {
			this->CreateInstance();
			this->SetupDebugMessenger();
			this->PickPhysicalDevice();
			this->CreateLogicalDevice();
			this->CreateCommandPool();
		}
		~DeviceManager() = default;

		vk::PhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice	; }
		vk::Device		   getDevice		() const { return m_Device			; }
		vk::Instance	   getInstance		() const { return m_Instance		; }
		vk::CommandPool	   getCommandPool	() const { return m_CommandPool		; }
		vk::Queue		   getPresentQueue	() const { return m_PresentQueue	; }
		vk::Queue		   getGraphicsQueue	() const { return m_GraphicsQueue	; }

	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPool();

	private:
		static void configureDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& create_info)noexcept;
		static std::vector<const char*> getRequiredExtensions();

	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data) {
			SAY("VALIDATION LAYER : " << p_callback_data->pMessage);
			return VK_FALSE;
		}

	private:
		Renderer* p_Renderer = nullptr;

		vk::raii::Context m_Context{};
		vk::raii::Instance m_Instance = VK_NULL_HANDLE;
		vk::raii::PhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		vk::raii::Device m_Device = VK_NULL_HANDLE;
		
		vk::raii::Queue m_GraphicsQueue = VK_NULL_HANDLE;
		vk::raii::Queue m_PresentQueue = VK_NULL_HANDLE;

		vk::raii::CommandPool m_CommandPool = VK_NULL_HANDLE;

#ifdef _DEBUG
		vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
#else
		// ..
#endif // _DEBUG
	};
}