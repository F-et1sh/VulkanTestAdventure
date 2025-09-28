#pragma once
#include "Vertices.h"
#include "GPUResourceManager.h"
#include "DeviceManager.h"
#include "RenderPassManager.h"

namespace VKTest {
	class PipelineManager {
	public:
		PipelineManager(DeviceManager* device_manager, RenderPassManager* render_pass_manager, GPUResourceManager* gpu_resource_manager);
		~PipelineManager() = default;

		void CreateGraphicsPipeline();
		void CreateDescriptorPool();

	private:
		static std::vector<char> readFile(const std::filesystem::path& path);
		vk::raii::ShaderModule createShaderModule(const std::vector<char>& code)const;

	private:
		DeviceManager* p_DeviceManager = nullptr;
		RenderPassManager* p_RenderPassManager = nullptr;
		GPUResourceManager* p_GPUResourceManager = nullptr;

		vk::raii::PipelineLayout m_PipelineLayout   = VK_NULL_HANDLE;
		vk::raii::Pipeline		 m_GraphicsPipeline = VK_NULL_HANDLE;
	};
}