#pragma once

namespace vk_test {
    class PathManager {
    public:
        VK_TEST_CLASS_NONCOPYABLE(PathManager)

        void init(const char* argv0, bool is_editor);

        static PathManager& Instance() {
            static PathManager path_manager;
            return path_manager;
        }

        [[nodiscard]] const std::filesystem::path& getExecutablePath() const noexcept {
            return m_ExecutablePath;
        }

        [[nodiscard]] const std::filesystem::path& getAssetsPath() const noexcept {
            return m_AssetsPath;
        }

        [[nodiscard]] std::filesystem::path getApplicationPath() const {
            return this->getAssetsPath() / L"Application";
        }

    private:
        VK_TEST_CLASS_DEFAULT(PathManager)

        std::filesystem::path m_ExecutablePath;
        std::filesystem::path m_AssetsPath;
    };

    inline static PathManager& PATH = PathManager::Instance();
} // namespace vk_test
