#pragma once
#include "Vertex.h"

constexpr static std::array<const char*, 4> DEVICE_EXTENSIONS = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName
};

constexpr static std::array<const char*, 1> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

constexpr static int MAX_FRAMES_IN_FLIGHT = 2;

class Application {
public:
    Application() = default;
    ~Application() = default;

    inline void Run() {
        this->InitializeWindow();

        this->CreateInstance();
        this->CreateSurface();
        this->PickPhysicalDevice();
        this->CreateLogicalDevice();
        this->CreateSwapchain();
        this->CreateImageViews();
        this->CreateGraphicsPipeline();
        this->CreateCommandPool();
        this->CreateCommandBuffers();
        this->CreateSyncObjects();
        this->CreateVertexBuffer();

        this->MainLoop();

        this->Release();
    }

private:
    void InitializeWindow();
    void CreateInstance();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapchain();
    void CreateImageViews();
    void CreateGraphicsPipeline();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void CreateVertexBuffer();
    void MainLoop();
    void Release();

private:
    void EnableValidationLayers(vk::InstanceCreateInfo& create_info)const;
    
    [[nodiscard]] uint32_t FindQueueFamilies()const;

    [[nodiscard]] vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats)const;
    [[nodiscard]] vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes)const;
    [[nodiscard]] vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)const;

    [[nodiscard]] std::vector<char> LoadShader(const std::filesystem::path& path)const;
    [[nodiscard]] vk::raii::ShaderModule CreateShaderModule(const std::vector<char>& code)const;

    void RecordCommandBuffer(vk::raii::CommandBuffer& command_buffer, uint32_t image_index);
    
    void TransitionImageLayout(
        vk::raii::CommandBuffer& command_buffer,
        uint32_t image_index,
        vk::ImageLayout old_layout,
        vk::ImageLayout new_layout,
        vk::AccessFlags2 src_access_mask,
        vk::AccessFlags2 dst_access_mask,
        vk::PipelineStageFlags2 src_stage_mask,
        vk::PipelineStageFlags2 dst_stage_mask
    )const;

    void DrawFrame();

    void ReleaseSwapchain();
    void RecreateSwapchain();

    uint32_t FindMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags properties)const;

private:
    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
    GLFWwindow* m_Window = nullptr;

private:
    vk::raii::Context m_Context = {};

    vk::raii::Instance m_Instance = nullptr;
    
    vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;
    vk::raii::Device m_Device = nullptr;
    
    uint32_t m_GraphicsQueueFamilyIndex = 0;
    uint32_t m_PresentQueueFamilyIndex = 0;

    vk::raii::Queue m_GraphicsQueue = nullptr;
    vk::raii::Queue m_PresentQueue = nullptr;
    
    vk::raii::SurfaceKHR m_Surface = nullptr;

    vk::raii::SwapchainKHR m_Swapchain = nullptr;
    std::vector<vk::Image> m_SwapchainImages;

    vk::Format m_SwapchainImageFormat = vk::Format::eUndefined;
    vk::Extent2D m_SwapchainExtent = {};

    std::vector<vk::raii::ImageView> m_SwapchainImageViews;

    vk::raii::PipelineLayout m_PipelineLayout = nullptr;
    vk::raii::Pipeline m_GraphicsPipeline = nullptr;

    vk::raii::CommandPool m_CommandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> m_CommandBuffers;

    std::vector<vk::raii::Semaphore> m_PresentCompleteSemaphores;
    std::vector<vk::raii::Semaphore> m_RenderFinishedSemaphores;
    std::vector<vk::raii::Fence> m_InFlightFences;

    bool m_FramebufferResized = false;

    uint32_t m_CurrentFrame = 0;

    vk::raii::Buffer m_VertexBuffer = nullptr;
    vk::raii::DeviceMemory m_VertexBufferMemory = nullptr;
};