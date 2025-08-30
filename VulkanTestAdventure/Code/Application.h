#pragma once

class Application {
public:
    Application() = default;
    ~Application() = default;

    inline void Run() {
        this->InitializeWindow();
        this->CreateInstance();
        this->PickPhysicalDevice();
        this->CreateLogicalDevice();

        std::cerr << m_PhysicalDevice.getProperties().deviceName;
    }

private:
    void InitializeWindow();
    void CreateInstance();
    void PickPhysicalDevice();
    void CreateLogicalDevice();

private:
    void EnableValidationLayers(vk::InstanceCreateInfo& create_info)const;
    uint32_t FindQueueFamilies()const;

private:
    GLFWwindow* m_Window = nullptr;

private:
    vk::raii::Context m_Context;

    vk::raii::Instance m_Instance = nullptr;
    vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;
    vk::raii::Device m_Device = nullptr;
    vk::raii::Queue m_GraphicsQueue = nullptr;
};