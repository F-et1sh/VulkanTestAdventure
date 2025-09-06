#pragma once
#include "Vertex.h"

struct UniformBufferObject {
    glm::mat4 model = glm::mat4();
    glm::mat4 view = glm::mat4();
    glm::mat4 proj = glm::mat4();

    UniformBufferObject() = default;
    ~UniformBufferObject() = default;

    constexpr UniformBufferObject(const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj)
        : model(model), view(view), proj(proj) {}
};

constexpr static std::array<const char*, 4> DEVICE_EXTENSIONS = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName
};  

constexpr static std::array VALIDATION_LAYERS = {
    "VK_LAYER_NV_optimus",
    "VK_LAYER_NV_present",
    "VK_LAYER_RENDERDOC_Capture",
    "VK_LAYER_VALVE_steam_overlay",
    "VK_LAYER_VALVE_steam_fossilize",
    "VK_LAYER_LUNARG_api_dump",
    "VK_LAYER_LUNARG_gfxreconstruct",
    "VK_LAYER_KHRONOS_synchronization2",
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_monitor",
    "VK_LAYER_LUNARG_screenshot",
    "VK_LAYER_KHRONOS_profiles",
    "VK_LAYER_KHRONOS_shader_object",
    "VK_LAYER_LUNARG_crash_diagnostic",
};

constexpr static int MAX_FRAMES_IN_FLIGHT = 2;

constexpr static auto REQUIRED_QUEUE_FAMILY_FLAGS = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;

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
        this->CreateDescriptorSetLayout();
        this->CreateGraphicsPipeline();
        this->CreateCommandPool();
        this->CreateCommandBuffers();
        this->CreateSyncObjects();
        this->CreateTextureImage();
        this->CreateTextureImageView();
        this->CreateTextureSampler();
        this->CreateVertexBuffer();
        this->CreateIndexBuffer();
        this->CreateUniformBuffers();
        this->CreateDescriptorPool();
        this->CreateDescriptorSets();

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
    void CreateDescriptorSetLayout();
    void CreateGraphicsPipeline();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void CreateTextureImage();
    void CreateTextureImageView();
    void CreateTextureSampler();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
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

    void CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& buffer_memory)const;
    void CopyBuffer(vk::raii::Buffer& src_buffer, vk::raii::Buffer& dst_buffer, vk::DeviceSize size);

    void UpdateUniformBuffer(uint32_t current_image);

    void CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image, vk::raii::DeviceMemory& image_memory)const;

    vk::raii::CommandBuffer BeginSingleTimeCommands()const;
    void EndSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer)const;

    void TransitionTextureImageLayout(const vk::raii::Image& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout)const;
    void CopyBufferToImage(const vk::raii::Buffer& buffer, const vk::raii::Image& image, uint32_t width, uint32_t height)const;

    vk::raii::ImageView CreateImageView(vk::raii::Image& image, vk::Format format)const;

    static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* callback_data, void*);

private:
    static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

private:
    GLFWwindow* m_Window = nullptr;

private:
    vk::raii::Context m_Context = {};

    vk::raii::Instance m_Instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT m_DebugMessenger = nullptr;
    
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

    vk::raii::DescriptorSetLayout m_DescriptorSetLayout = nullptr;
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

    vk::raii::Buffer m_IndexBuffer = nullptr;
    vk::raii::DeviceMemory m_IndexBufferMemory = nullptr;

    std::vector<vk::raii::Buffer> m_UniformBuffers;
    std::vector<vk::raii::DeviceMemory> m_UniformBuffersMemory;
    std::vector<void*> m_UniformBuffersMapped;

    vk::raii::DescriptorPool m_DescriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> m_DescriptorSets;

    vk::raii::Image m_TextureImage = nullptr;
    vk::raii::DeviceMemory m_TextureImageMemory = nullptr;
    vk::raii::ImageView m_TextureImageView = nullptr;
    vk::raii::Sampler m_TextureSampler = nullptr;
};