#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <unordered_set>
#include <fstream>
#include <filesystem>

class Application {
public:
    Application() = default;
    ~Application() = default;

    inline void Run() {
        this->CreateInstance();
    }

private:
    void InitializeWindow();
    void CreateInstance();

private:
    void EnableValidationLayers(vk::InstanceCreateInfo& create_info)const;

private:
    GLFWwindow* m_Window = nullptr;

private:
    vk::raii::Context m_Context;
    vk::raii::Instance m_Instance = nullptr;
};