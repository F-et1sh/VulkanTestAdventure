#pragma once
#include "QueueFamilyIndices.h"

namespace VKTest {
	class SwapchainManager; // forward declaration

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
		DeviceManager(SwapchainManager* swapchain_manager) : p_SwapchainManager{swapchain_manager} {}
		~DeviceManager() = default;

		void CreateInstance();
		void SetupDebugMessenger();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateCommandPool();
		void CreateCommandBuffers();

		const vk::raii::Device&		getDevice				()const noexcept { return m_Device; }
		
		vk::raii::Device&			getDevice				()noexcept { return m_Device; }
		vk::raii::Instance&			getInstance				()noexcept { return m_Instance; }
		vk::raii::CommandPool&		getCommandPool			()noexcept { return m_CommandPool; }
		vk::raii::Queue&			getPresentQueue			()noexcept { return m_PresentQueue; }
		vk::raii::Queue&			getGraphicsQueue		()noexcept { return m_GraphicsQueue; }
		vk::raii::PhysicalDevice&	getPhysicalDevice		()noexcept { return m_PhysicalDevice; }
		QueueFamilyIndices			getQueueFamilyIndices	()noexcept { return m_QueueFamilyIndices; }

	public:
		uint32_t findQueueFamilies(vk::PhysicalDevice device, vk::QueueFlagBits flags)const;
		vk::Format findDepthFormat()const;
		
	private:
		void configureDebugMessengerCreateInfo(vk::DebugUtilsMessengerCreateInfoEXT& create_info)const noexcept;
		std::vector<const char*> getRequiredExtensions()const;
		vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)const;

	private:
		// uses C-Style Vulkan API
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data) {
			// if not warning or error -> return
			if (!(message_severity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) &&
				!(message_severity & VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) return VK_FALSE;

			SAY("VALIDATION LAYER : " << callback_data->pMessage);

			return VK_FALSE;
		}

	private:
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