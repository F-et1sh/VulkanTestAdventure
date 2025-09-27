#pragma once

#define SAY(message) std::wcerr << std::endl << message << std::endl
#define SAY_withoutBreak(message) std::wcerr << message

#define RUNTIME_ERROR(message) throw std::runtime_error(message)

#define NODISCARD [[nodiscard]]

namespace VKTest {
    inline static void copy_file(const std::filesystem::path& from, const std::filesystem::path& to) {
        if (!std::filesystem::exists(to)) {
            std::filesystem::create_directories(to);
        }
        for (const auto& entry : std::filesystem::recursive_directory_iterator(from)) {
            const auto& destPath = to / std::filesystem::relative(entry.path(), from);
            if (std::filesystem::is_directory(entry.status())) {
                std::filesystem::create_directories(destPath);
            }
            else if (std::filesystem::is_regular_file(entry.status())) {
                std::filesystem::copy_file(entry.path(), destPath, std::filesystem::copy_options::overwrite_existing);
            }
        }
    }
    inline static void copy_if_new(const std::filesystem::path& from, const std::filesystem::path& to) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(from)) {
            auto relative_path = std::filesystem::relative(entry.path(), from);
            auto target_path = to / relative_path;

            if (entry.is_directory()) {
                std::filesystem::create_directories(target_path);
            }
            else if (!std::filesystem::exists(target_path) ||
                std::filesystem::last_write_time(entry) > std::filesystem::last_write_time(target_path)) {
                std::filesystem::copy_file(entry, target_path, std::filesystem::copy_options::overwrite_existing);
            }
        }
    }

    inline static std::filesystem::path generate_unique_filename(const std::filesystem::path& file_path) {
        std::filesystem::path path = file_path;
        std::filesystem::path stem = path.stem();
        std::filesystem::path extension = path.extension();
        int count = 1;

        while (std::filesystem::exists(path)) {
            path.replace_filename(stem.wstring() + L" (" + std::to_wstring(count) + L")" + extension.wstring());
            count++;
        }

        return path;
    }
}