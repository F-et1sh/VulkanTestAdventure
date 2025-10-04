#include "pch.h"
#include "Application.h"

constexpr glm::vec2 WINDOW_RESOLUTION = glm::vec2(1920, 1080);
constexpr std::string_view WINDOW_TITLE = "VKHppTest";
constexpr int WINDOW_MONITOR = -1; // not fullscreen

constexpr int SUCCESSFUL_EXIT = 0;
constexpr int FAILED_EXIT = -1;

int main(int argc, char* argv[]) {
	try {
		VKHppTest::PATH.init(argv[0], true); // instance of the PathManager
		
		std::unique_ptr<VKHppTest::Application> app = std::make_unique<VKHppTest::Application>(WINDOW_RESOLUTION, WINDOW_TITLE.data(), WINDOW_MONITOR);
		app->Loop();
	}
	catch (const std::exception& e) {

		SAY(e.what());

		std::ofstream file("output.txt");
		file << e.what();
		file.close();

		return FAILED_EXIT;
	}

	return SUCCESSFUL_EXIT;
}
