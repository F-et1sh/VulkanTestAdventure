#pragma once

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
    }

private:
    void InitializeWindow();
    void CreateInstance();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapchain();
    void CreateImageViews();

private:
    void EnableValidationLayers(vk::InstanceCreateInfo& create_info)const;
    
    uint32_t FindQueueFamilies()const;

    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats);
    vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes);
    vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

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
};