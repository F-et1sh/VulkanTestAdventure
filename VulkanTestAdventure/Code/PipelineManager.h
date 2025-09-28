#pragma once
#include "Vertices.h"
#include "DeviceManager.h"

namespace VKTest {
	class PipelineManager {
	public:
		PipelineManager(DeviceManager* device_manager);
		~PipelineManager() = default;

		void CreateGraphicsPipeline();

	private:
		static std::vector<char> readFile(const std::filesystem::path& path);
		vk::raii::ShaderModule createShaderModule(const std::vector<char>& code)const;

	private:
		DeviceManager* p_DeviceManager = nullptr;

		vk::raii::PipelineLayout m_PipelineLayout   = VK_NULL_HANDLE;
		vk::raii::Pipeline		 m_GraphicsPipeline = VK_NULL_HANDLE;
	};
}