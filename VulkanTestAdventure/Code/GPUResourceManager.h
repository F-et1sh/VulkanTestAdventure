#pragma once
#include "DeviceManager.h"
#include "RenderPassManager.h"
#include "Image.h"

namespace VKTest {
	class SwapchainManager; // forward declaration

	// define a structure to hold per-object data
	struct GameObject {
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 scale = { 1.0f, 1.0f, 1.0f };

		std::vector<vk::raii::Buffer> uniform_buffers;
		std::vector<vk::raii::DeviceMemory> uniform_buffers_memory;
		std::vector<void*> uniform_buffers_mapped;

		std::vector<vk::raii::DescriptorSet> descriptor_sets;

		glm::mat4 getModelMatrix()const noexcept {
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, position);
			model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, scale);
			return model;
		}
	};

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
	};

	class GPUResourceManager {
	public:
		GPUResourceManager(DeviceManager* device_manager, SwapchainManager* swapchain_manager, RenderPassManager* render_pass_manager);
		~GPUResourceManager() = default;

		void CreateDescriptorSetLayout();
		void CreateColorResources();
		void CreateDepthResources();
		void CreateDescriptorPool();
		void CreateDescriptorSets();
		void CreateUniformBuffers();
		void CreateSyncObjects();

		inline vk::raii::DescriptorSetLayout& getDescriptorSetLayout()noexcept { return m_DescriptorSetLayout; }
		inline Image& getColorImage()noexcept { return m_ColorImage; }
		inline Image& getDepthImage()noexcept { return m_DepthImage; }

	private:
		void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& buffer_memory);

	private:
		DeviceManager*						 p_DeviceManager = nullptr;
		SwapchainManager*					 p_SwapchainManager = nullptr;
		RenderPassManager*					 p_RenderPassManager = nullptr;

		vk::raii::DescriptorSetLayout		 m_DescriptorSetLayout = VK_NULL_HANDLE;
		vk::raii::DescriptorPool			 m_DescriptorPool = VK_NULL_HANDLE;
		std::vector<vk::raii::DescriptorSet> m_DescriptorSets;

		Image								 m_ColorImage;
		Image								 m_DepthImage;

		uint32_t							 m_MipLevels = 0;
		Image								 m_TextureImage;
		vk::raii::Sampler					 m_TextureSampler = VK_NULL_HANDLE;

		// array of game objects to render
		std::array<GameObject, MAX_FRAMES_IN_FLIGHT>  m_GameObjects;

		std::vector<vk::raii::Semaphore> m_ImageAvailableSemaphores;
		std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;
		std::vector<vk::raii::Fence> m_InFlightFences;
	};
}