#include "pch.h"
#include "Application.hpp"

#include "element_camera.hpp"
#include "element_default_title.hpp"
#include "element_default_menu.hpp"

constexpr inline static glm::vec2        WINDOW_RESOLUTION = glm::vec2(1920, 1080);
constexpr inline static std::string_view WINDOW_TITLE      = "VKTest";
constexpr inline static int              WINDOW_MONITOR    = -1; // not fullscreen

constexpr inline static int SUCCESSFUL_EXIT = 0;
constexpr inline static int FAILED_EXIT     = -1;

int main(int argc, char* argv[]) {
    try {
        vk_test::PATH.init(argv[0], true); // instance of the PathManager
    }
    catch (const std::exception& e) {
        VK_TEST_SAY(e.what());

        std::ofstream file(vk_test::PATH.getExecutablePath().string() + "PATH_output.txt");
        file << e.what();
        file.close();

        return FAILED_EXIT;
    }

    std::unique_ptr<vk_test::Application> app     = std::make_unique<vk_test::Application>(WINDOW_RESOLUTION, WINDOW_TITLE.data(), WINDOW_MONITOR);
    std::unique_ptr<vk_test::Context>     context = std::make_unique<vk_test::Context>();

    try {
        vk_test::ApplicationCreateInfo application_create_info{};

        // Setting up the Vulkan context, instance and device extensions
        VkPhysicalDeviceShaderObjectFeaturesEXT          shader_object_features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT };
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accel_feature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR    rt_pipeline_feature{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };

        vk_test::ContextInitInfo vk_setup{
            .instance_extensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME },
            .device_extensions   = {
                { VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME },
                { VK_EXT_SHADER_OBJECT_EXTENSION_NAME, &shader_object_features },
                { VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, &accel_feature },     // To build acceleration structures
                { VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, &rt_pipeline_feature }, // To use vkCmdTraceRaysKHR
                { VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME },                   // Required by ray tracing pipeline
            },
        };

        uint32_t     count{};
        const char** ext = glfwGetRequiredInstanceExtensions(&count);
        for (uint32_t i = 0; i < count; i++) {
            vk_setup.instance_extensions.emplace_back(ext[i]);
        }

        if (!application_create_info.headless) {
            vk_test::addSurfaceExtensions(vk_setup.instance_extensions, &vk_setup.device_extensions);
        }

        context->Initialize(vk_setup);

        application_create_info.name            = "VulkanTest";
        application_create_info.instance        = context->getInstance();
        application_create_info.device          = context->getDevice();
        application_create_info.physical_device = context->getPhysicalDevice();
        application_create_info.queues          = context->getQueueInfos();

        // Elements added to the application
        //auto tutorial    = std::make_shared<Rt12InfinitePlane>();
        auto element_camera  = std::make_shared<vk_test::ElementCamera>();
        //auto window_title = std::make_shared<vk_test::ElementDefaultWindowTitle>();
        //auto window_menu  = std::make_shared<vk_test::ElementDefaultMenu>();
        auto camera_manipulator    = tutorial->getCameraManipulator();
        element_camera->setCameraManipulator(camera_manipulator);

        //// Add elements
        //app->addElement(window_menu);
        //app->addElement(window_title);
        app->addElement(element_camera);
        //app->addElement(tutorial);

        app->Initialize(application_create_info);
        app->Loop();
        app->Release();

        context->Release();
    }
    catch (const std::exception& e) {
        VK_TEST_SAY(e.what());

        std::ofstream file(vk_test::PATH.getExecutablePath().string() + "output.txt");
        file << e.what();
        file.close();

        return FAILED_EXIT;
    }

    return SUCCESSFUL_EXIT;
}
