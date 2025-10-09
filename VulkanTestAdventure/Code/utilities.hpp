#pragma once

#define VKTEST_SAY(message) std::wcerr << std::endl \
                                << (message) << std::endl
#define VKTEST_SAY_WITHOUT_BREAK(message) std::wcerr << message

#define VKTEST_RUNTIME_ERROR(message) throw std::runtime_error(message)

#define VKTEST_NODISCARD [[nodiscard]]

namespace vk_test {
    inline static void CopyFile(const std::filesystem::path& from, const std::filesystem::path& to) {
        if (!std::filesystem::exists(to)) {
            std::filesystem::create_directories(to);
        }
        for (const auto& entry : std::filesystem::recursive_directory_iterator(from)) {
            const auto& dest_path = to / std::filesystem::relative(entry.path(), from);
            if (std::filesystem::is_directory(entry.status())) {
                std::filesystem::create_directories(dest_path);
            }
            else if (std::filesystem::is_regular_file(entry.status())) {
                std::filesystem::copy_file(entry.path(), dest_path, std::filesystem::copy_options::overwrite_existing);
            }
        }
    }
    inline static void CopyIfNew(const std::filesystem::path& from, const std::filesystem::path& to) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(from)) {
            auto relative_path = std::filesystem::relative(entry.path(), from);
            auto target_path   = to / relative_path;

            if (entry.is_directory()) {
                std::filesystem::create_directories(target_path);
            }
            else if (!std::filesystem::exists(target_path) ||
                     std::filesystem::last_write_time(entry) > std::filesystem::last_write_time(target_path)) {
                std::filesystem::copy_file(entry, target_path, std::filesystem::copy_options::overwrite_existing);
            }
        }
    }

    inline static auto GenerateUniqueFilename(const std::filesystem::path& file_path) -> std::filesystem::path {
        std::filesystem::path path      = file_path;
        std::filesystem::path stem      = path.stem();
        std::filesystem::path extension = path.extension();
        int                   count     = 1;

        while (std::filesystem::exists(path)) {
            path.replace_filename(stem.wstring() + L" (" + std::to_wstring(count) + L")" + extension.wstring());
            count++;
        }

        return path;
    }
} // namespace VKTest
