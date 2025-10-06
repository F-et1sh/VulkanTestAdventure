#pragma once
#include "VkHpp_QueueFamilyIndices.h"

// все комментарии на русском нужны для описания формата. Файл уже не актуальный, но хорошо описывает что куда и как пихать

// namespace тут
namespace VKHppTest {
	// писать тут и обязательно подписать '// forward declaration' или '/* forward declarations *\'
	class SwapchainManager; // forward declaration

	class DeviceManager {
	private: // обязательно написать public или private. Чаще всего public и сразу после него конструктор и деконструктор, но тут нужно было включить глобальные переменные, поэтому тут стоит public
#ifdef _DEBUG
		constexpr static bool ENABLE_VALIDATION_LAYERS = true; // глобальные переменные написаны капсом
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
		DeviceManager(SwapchainManager* swapchain_manager) : p_SwapchainManager{swapchain_manager} {} // конструктор всегда рядом с деструктором
		~DeviceManager() = default; // деструктор пишется после конструктора

		// тут блок для методов инициализации, которые нужно вызывать в строгом порядке. Пишутся с заглавной буквы
		void CreateInstance();
		void SetupDebugMessenger();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPool();
		void CreateCommandBuffers();

		// тут блок для геттеров, формат : get + MemberName
		const vk::raii::Device&					getDevice				()const noexcept { return m_Device; }
		vk::raii::Device&						getDevice				()noexcept { return m_Device; }
		vk::raii::Instance&						getInstance				()noexcept { return m_Instance; }
		vk::raii::CommandPool&					getCommandPool			()noexcept { return m_CommandPool; }
		vk::raii::Queue&						getPresentQueue			()noexcept { return m_PresentQueue; }
		vk::raii::Queue&						getGraphicsQueue		()noexcept { return m_GraphicsQueue; }
		vk::raii::PhysicalDevice&				getPhysicalDevice		()noexcept { return m_PhysicalDevice; }
		QueueFamilyIndices						getQueueFamilyIndices	()noexcept { return m_QueueFamilyIndices; }
		std::vector<vk::raii::CommandBuffer>&	getCommandBuffers		()noexcept { return m_CommandBuffers; }

	public: // тут уже следующий блок для всяких утилит, пишутся с маленькой буквы, должы быть const
		uint32_t findQueueFamilies(vk::PhysicalDevice device, vk::QueueFlagBits flags)const;
		vk::Format findDepthFormat()const;
		uint32_t findMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties)const;
		vk::raii::CommandBuffer beginSingleTimeCommands()const;
		void endSingleTimeCommands(vk::raii::CommandBuffer& command_buffer)const;
		
	private: // тут утилиты только для класса
		void configureDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& create_info)const noexcept;
		std::vector<const char*> getRequiredExtensions()const;
		vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)const;

	private: // тут callback-и
		// uses C-Style Vulkan API
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
			// if not warning or error -> return
			if (!(message_severity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) &&
				!(message_severity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) return VK_FALSE;

			SAY("VALIDATION LAYER : " << callback_data->pMessage);

			return VK_FALSE;
		}

	private: // тут все переменные. m_MemberName - просто member, p_PtrName - указатель
		SwapchainManager* p_SwapchainManager = nullptr;

		vk::raii::Context m_Context{};
		vk::raii::Instance m_Instance = VK_NULL_HANDLE;
		vk::raii::PhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		vk::raii::Device m_Device = VK_NULL_HANDLE;

		QueueFamilyIndices m_QueueFamilyIndices{};
		
		vk::raii::Queue m_GraphicsQueue = VK_NULL_HANDLE;
		vk::raii::Queue m_PresentQueue = VK_NULL_HANDLE;

		vk::raii::CommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<vk::raii::CommandBuffer> m_CommandBuffers;

#ifdef _DEBUG
		vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
#else
		// ..
#endif // _DEBUG
	};
}
