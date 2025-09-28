#pragma once

namespace VKTest {
	class Image {
	public:
		Image(uint32_t width,
			uint32_t height,
			uint32_t mip_levels,
			vk::SampleCountFlagBits num_samples,
			vk::Format format,
			vk::ImageAspectFlags aspect_flags,
			vk::ImageTiling tiling,
			vk::ImageUsageFlags usage,
			vk::MemoryPropertyFlags properties,
			vk::raii::Device& device);
		Image() = default;
		~Image() = default;

		void Initialize(uint32_t width,
			uint32_t height,
			uint32_t mip_levels,
			vk::SampleCountFlagBits num_samples,
			vk::Format format,
			vk::ImageAspectFlags aspect_flags,
			vk::ImageTiling tiling,
			vk::ImageUsageFlags usage,
			vk::MemoryPropertyFlags properties,
			vk::raii::Device& device);

	private:
		static vk::raii::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags, uint32_t mip_levels, vk::raii::Device& device);
		static void createImage(
			uint32_t width, 
			uint32_t height, 
			uint32_t mip_levels, 
			vk::SampleCountFlagBits num_samples, 
			vk::Format format, 
			vk::ImageTiling tiling, 
			vk::ImageUsageFlags usage, 
			vk::MemoryPropertyFlags properties, 
			vk::raii::Image& image, 
			vk::raii::DeviceMemory& image_memory, 
			vk::raii::Device& device);

	private:
		vk::raii::Image	m_Image = VK_NULL_HANDLE;
		vk::raii::DeviceMemory m_ImageMemory = VK_NULL_HANDLE;
		vk::raii::ImageView	m_ImageView = VK_NULL_HANDLE;
	};
}