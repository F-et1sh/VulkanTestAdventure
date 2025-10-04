#pragma once

namespace VKHppTest {
	class PathManager {
	public:
		CLASS_NONCOPYABLE(PathManager)

		void init(const char* argv0, bool is_editor);

		static PathManager& Instance() {
			static PathManager path_manager;
			return path_manager;
		}

	public:
		inline const std::filesystem::path& get_executable_path()const noexcept {
			return m_ExecutablePath;
		}

		inline const std::filesystem::path& get_assets_path() const noexcept {
			return m_AssetsPath;
		}

		inline std::filesystem::path get_application_path()const {
			return this->get_assets_path() / L"Application";
		}

	private:
		CLASS_DEFAULT(PathManager)

	private:
		std::filesystem::path m_ExecutablePath;
		std::filesystem::path m_AssetsPath;
	};

	inline static PathManager& PATH = PathManager::Instance();
}