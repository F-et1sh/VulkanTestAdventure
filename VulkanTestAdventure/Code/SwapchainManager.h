#pragma once
#include "Window.h"
#include "DeviceManager.h"
#include "QueueFamilyIndices.h"

namespace VKTest {
	struct SwapChainSupportDetails {
		vk::SurfaceCapabilitiesKHR capabilities;
		std::vector<vk::SurfaceFormatKHR> formats;
		std::vector<vk::PresentModeKHR> present_modes;

		SwapChainSupportDetails() = default;
		~SwapChainSupportDetails() = default;
	};

	class SwapchainManager {
	public:
		SwapchainManager(DeviceManager* device_manager, Window* window);
		~SwapchainManager() = default;

		void CreateSurface();
		void CreateSwapchain();
		void CreateImageViews();

		inline vk::raii::SurfaceKHR& getSurface()noexcept { return m_Surface; }
		inline vk::Format getFormat()const noexcept { return m_SwapchainImageFormat; }
		inline vk::Extent2D getExtent()const noexcept { return m_SwapchainExtent; }

	private:
		SwapChainSupportDetails querySwapchainSupport(vk::PhysicalDevice device)const;
		vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats)const;
		vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes)const;
		VkExtent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)const;

	private:
		DeviceManager* p_DeviceManager = nullptr;
		Window* p_Window = nullptr;
		
		vk::raii::SurfaceKHR m_Surface = VK_NULL_HANDLE;

		vk::raii::SwapchainKHR m_Swapchain = VK_NULL_HANDLE;

		vk::Format m_SwapchainImageFormat{};
		vk::Extent2D m_SwapchainExtent{};

		std::vector<vk::Image> m_SwapchainImages;
		std::vector<vk::raii::ImageView> m_SwapchainImageViews;
		std::vector<vk::raii::Framebuffer> m_SwapchainFramebuffers;
	};
}