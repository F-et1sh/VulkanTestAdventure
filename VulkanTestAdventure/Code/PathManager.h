#pragma once

namespace vk_test {
    class PathManager {
    public:
        PathManager(const PathManager&)            = delete;
        PathManager& operator=(const PathManager&) = delete;

        void Init(const char* argv0, bool is_editor);

        static PathManager& Instance() {
            static PathManager PATH_MANAGER;
            return PATH_MANAGER;
        }

        [[nodiscard]] const std::filesystem::path& GetExecutablePath() const noexcept {
            return m_ExecutablePath;
        }

        [[nodiscard]] const std::filesystem::path& GetAssetsPath() const noexcept {
            return m_AssetsPath;
        }

        [[nodiscard]] const std::filesystem::path& GetApplicationPath() const {
            return this->GetAssetsPath() / L"Application";
        }

    private:
        VKTEST_CLASS_DEFAULT(PathManager)

        std::filesystem::path m_ExecutablePath;
        std::filesystem::path m_AssetsPath;
    };

    inline static PathManager& PATH = PathManager::Instance();
} // namespace vk_test
