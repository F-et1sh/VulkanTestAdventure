#pragma once
#include "RenderMesh.h"

namespace VKTest {
    /* forward declarations */
    class DeviceManager;
    class RenderPassManager;
    class SwapchainManager;
    class RenderMesh;

    class PipelineManager {
    public:
        PipelineManager(DeviceManager* device_manager, RenderPassManager* render_pass_manager, SwapchainManager* swapchain_manager, RenderMesh* render_mesh)
            : p_DeviceManager{ device_manager }, p_RenderPassManager{ render_pass_manager }, p_SwapchainManager{ swapchain_manager }, p_RenderMesh{ render_mesh } {}
        ~PipelineManager() { this->Release(); }

        void Release();

        void CreateDescriptorSetLayout();
        void CreateGraphicsPipeline();
        void CreateDescriptorPool();
        void CreateDescriptorSets();

        VkPipelineLayout getPipelineLayout() const noexcept { return m_PipelineLayout; }
        VkPipeline       getGraphicsPipeline() const noexcept { return m_GraphicsPipeline; }

    private:
        static std::vector<char> readFile(const std::filesystem::path& path);
        VkShaderModule           createShaderModule(const std::vector<char>& code) const;

    private:
        DeviceManager*     p_DeviceManager     = nullptr;
        RenderPassManager* p_RenderPassManager = nullptr;
        SwapchainManager*  p_SwapchainManager  = nullptr;
        RenderMesh*        p_RenderMesh        = nullptr;

        VkDescriptorSetLayout m_DescriptorSetLayout{};
        VkDescriptorPool      m_DescriptorPool{};

        VkPipelineLayout m_PipelineLayout{};
        VkPipeline       m_GraphicsPipeline{};
    };
} // namespace VKTest
