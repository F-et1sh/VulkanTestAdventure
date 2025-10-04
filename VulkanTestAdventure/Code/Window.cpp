#include "pch.h"
#include "Window.h"

VKHppTest::Window::Window(const glm::vec2& window_resolution, const std::string& window_title, int window_monitor) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    this->CreateWindow(window_resolution, window_title, window_monitor);
}

VKHppTest::Window::~Window() {
    m_IsRunning = false;

    glfwDestroyWindow(p_GLFWWindow);
    glfwTerminate();
}

void VKHppTest::Window::CreateWindow(const glm::vec2& window_resolution, const std::string& window_title, int window_monitor) {
    GLFWmonitor* monitor = nullptr;

    int monitor_count = 0;
    auto monitors = glfwGetMonitors(&monitor_count);
    if (window_monitor > 0 && window_monitor < monitor_count) monitor = monitors[window_monitor];

    p_GLFWWindow = glfwCreateWindow(window_resolution.x, window_resolution.y, window_title.data(), monitor, nullptr);
    if (!p_GLFWWindow) RUNTIME_ERROR("ERROR : Failed to create GLFW window");

    glfwSetWindowUserPointer(p_GLFWWindow, this);
    glfwSetFramebufferSizeCallback(p_GLFWWindow, framebufferResizeCallback);

    m_IsRunning = true;
}

void VKHppTest::Window::ClearColor(glm::vec4 color) const noexcept {

}

void VKHppTest::Window::ClearColor(float r, float g, float b, float a) const noexcept {
    
}

void VKHppTest::Window::ClearScreen(unsigned int buffer_bit) const noexcept {

}

void VKHppTest::Window::SwapBuffers() const noexcept {

}

void VKHppTest::Window::PollEvents()const noexcept {
    glfwPollEvents();
}
