#include "pch.h"
#include "Application.hpp"

vk_test::Application::~Application() {
    m_Context.Release();
}

void vk_test::Application::Initialize() {
    // Setting up the Vulkan context, instance and device extensions
    VkPhysicalDeviceShaderObjectFeaturesEXT          shader_object_features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_feature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR    rt_pipeline_feature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };

    ContextInitInfo vk_setup{
        .instance_extensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME },
        .device_extensions   = {
            { VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME },
            { VK_EXT_SHADER_OBJECT_EXTENSION_NAME, &shader_object_features },
            { VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, &accel_feature },     // To build acceleration structures
            { VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, &rt_pipeline_feature }, // To use vkCmdTraceRaysKHR
            { VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME },                   // Required by ray tracing pipeline
        },
    };

    vk_setup.instance_extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
    vk_setup.instance_extensions.emplace_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    vkSetup.instance_extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
    vkSetup.instance_extensions.emplace_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
    vkSetup.instance_extensions.emplace_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
    vkSetup.instance_extensions.emplace_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    vkSetup.instance_extensions.emplace_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_IOS_MVK)
    vkSetup.instance_extensions.emplace_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    vkSetup.instance_extensions.emplace_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

    vk_setup.device_extensions.push_back({ VK_KHR_SWAPCHAIN_EXTENSION_NAME });

    m_Context.Initialize(vk_setup);
}

void vk_test::Application::Loop() {
    while (glfwWindowShouldClose(m_Window.getGLFWWindow()) == 0) {
        vk_test::Window::PollEvents();
    }
}
