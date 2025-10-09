#pragma once

namespace vk_test {
    class PathManager {
    public:
        VKTEST_CLASS_NONCOPYABLE(PathManager)

        void Init(const char* argv0, bool is_editor);

        static auto Instance() -> PathManager& {
            static PathManager PATH_MANAGER;
            return PATH_MANAGER;
        }

    
        [[nodiscard]] auto GetExecutablePath() const noexcept -> const std::filesystem::path& {
            return m_ExecutablePath;
        }

        [[nodiscard]] auto GetAssetsPath() const noexcept -> const std::filesystem::path& {
            return m_AssetsPath;
        }

        [[nodiscard]] auto GetApplicationPath() const -> std::filesystem::path {
            return this->GetAssetsPath() / L"Application";
        }

    private:
        VKTEST_CLASS_DEFAULT(PathManager)

    
        std::filesystem::path m_ExecutablePath;
        std::filesystem::path m_AssetsPath;
    };

    inline static PathManager& PATH = PathManager::Instance();
} // namespace VKTest
