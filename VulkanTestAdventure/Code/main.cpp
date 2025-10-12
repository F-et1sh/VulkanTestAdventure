#include "pch.h"
#include "Application.h"

constexpr inline static glm::vec2        WINDOW_RESOLUTION = glm::vec2(1920, 1080);
constexpr inline static std::string_view WINDOW_TITLE      = "VKTest";
constexpr inline static int              WINDOW_MONITOR    = -1; // not fullscreen

constexpr inline static int SUCCESSFUL_EXIT = 0;
constexpr inline static int FAILED_EXIT     = -1;

int main(int argc, char* argv[]) {
    try {
        VKTest::PATH.init(argv[0], true); // instance of the PathManager

        std::unique_ptr<VKTest::Application> app = std::make_unique<VKTest::Application>(WINDOW_RESOLUTION, WINDOW_TITLE.data(), WINDOW_MONITOR);
        app->Loop();
    }
    catch (const std::exception& e) {
        VK_TEST_SAY(e.what());

        std::ofstream file("F:/Windows/Desktop/VulkanTestAdventure/output.txt");
        file << e.what();
        file.close();

        return FAILED_EXIT;
    }

    return SUCCESSFUL_EXIT;
}
