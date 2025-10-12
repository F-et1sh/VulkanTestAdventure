#pragma once
#include "Vertices.h"

namespace VKTest {
    /* forward declaration */
    class DeviceManager;
    class RenderPassManager;
    class SwapchainManager;

    class PipelineManager {
    public:
        PipelineManager(DeviceManager* device_manager, RenderPassManager* render_pass_manager, SwapchainManager* swapchain_manager)
            : p_DeviceManager{ device_manager }, p_RenderPassManager{ render_pass_manager }, p_SwapchainManager{ swapchain_manager } {}
        ~PipelineManager() = default;

        void CreateDescriptorSetLayout();
        void CreateGraphicsPipeline();

    private:
        static std::vector<char> readFile(const std::filesystem::path& path);
        VkShaderModule           createShaderModule(const std::vector<char>& code) const;

    private:
        DeviceManager*     p_DeviceManager     = nullptr;
        RenderPassManager* p_RenderPassManager = nullptr;
        SwapchainManager*  p_SwapchainManager  = nullptr;

        VkDescriptorSetLayout m_DescriptorSetLayout{};

        VkPipelineLayout m_PipelineLayout{};
        VkPipeline       m_GraphicsPipeline{};
    };
} // namespace VKTest
