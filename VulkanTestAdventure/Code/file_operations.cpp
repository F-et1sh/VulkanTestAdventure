/*
 * Copyright (c) 2023-2025, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2023-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
* Modified by Farrakh
* 2025
*/

#include "pch.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <libloaderapi.h> // GetModuleFileNameA
#include <stringapiset.h> // WideChartoMultiByte
#else
#include <limits.h>    // PATH_MAX
#include <strings.h>   // strcasecmp
#include <sys/types.h> // ssize_t
#include <unistd.h>    // readlink
#endif

#include "file_operations.hpp"

#include <fstream>
#include <limits>
#include <tuple>

std::filesystem::path vk_test::findFile(const std::filesystem::path&              filename,
                                        const std::vector<std::filesystem::path>& search_paths,
                                        bool                                      report_error) {
    for (const auto& path : search_paths) {
        const std::filesystem::path file_path = path / filename;
        if (std::filesystem::exists(file_path)) {
            return file_path;
        }
    }
    VK_TEST_SAY("File not found : \n"
                << utf8FromPath(filename).c_str());
    VK_TEST_SAY("Searched under : \n");
    for (const auto& path : search_paths) {
        VK_TEST_SAY("  %s\n"
                    << utf8FromPath(path).c_str());
    }
    return std::filesystem::path();
}

std::string vk_test::loadFile(const std::filesystem::path& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file) {
        VK_TEST_SAY("Could not open file : \n"
                    << utf8FromPath(file_path).c_str());
        return "";
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        VK_TEST_SAY("Error reading file : \n"
                    << utf8FromPath(file_path).c_str());
        return "";
    }

    return std::string(buffer.begin(), buffer.end());
}

std::filesystem::path vk_test::getExecutablePath() {
    return PATH.getExecutablePath();
}

std::string vk_test::utf8FromPath(const std::filesystem::path& path) noexcept {
    try {
#ifdef _WIN32
        // On Windows, paths are UTF-16, possibly with unpaired surrogates.
        // There's several possible routes to convert to UTF-8; we use WideCharToMultiByte because
        // - <codecvt> is deprecated and scheduled to be removed from C++ (see https://github.com/microsoft/STL/issues/443)
        // -  _wcstombs_s_l requires a locale object
        // We choose to reject unpaired surrogates using WC_ERR_INVALID_CHARS,
        // in case downstream code expects fully valid UTF-8.
        if (path.empty()) // Convert empty strings without producing errors
        {
            return "";
        }
        const wchar_t* utf16_str        = path.c_str();
        const size_t   utf16_characters = wcslen(utf16_str);
        // Cast to int for WideCharToMultiByte
        if (utf16_characters > std::numeric_limits<int>::max()) {
            VK_TEST_SAY(__func__ << " : Input had too many characters to store in an int.");
            return "";
        }
        const int utf16_characters_i = static_cast<int>(utf16_characters);
        // Get output size (does not include terminating null since we specify cchWideChar)
        const int utf8_bytes = WideCharToMultiByte(CP_UTF8,              // CodePage
                                                   WC_ERR_INVALID_CHARS, // dwFlags
                                                   utf16_str,            // lpWideCharStr
                                                   utf16_characters_i,   // cchWideChar
                                                   nullptr,
                                                   0, // Output
                                                   nullptr,
                                                   nullptr); // lpDefaultChar, lpUsedDefaultChar
        // WideCharToMultiByte returns 0 on failure. Check for that plus negative
        // values (which chould never happen):
        if (utf8_bytes <= 0) {
            VK_TEST_SAY(__func__ << " : WideCharToMultiByte failed. The path is probably not valid UTF-16.");
            return "";
        }
        std::string result(utf8_bytes, char(0));
        std::ignore = WideCharToMultiByte(CP_UTF8, 0, utf16_str, utf16_characters_i, result.data(), utf8_bytes, nullptr, nullptr);
        return result;
#else
        // On other platforms, paths are UTF-8 already.
        // Catch exceptions just to make this function 100% noexcept.
        static_assert(std::is_same_v<std::filesystem::path::string_type::value_type, char>);
        return path.string();
#endif
    }
    catch (const std::exception& e) // E.g. out of memory
    {
        VK_TEST_SAY(__func__ << " : threw an exception : " << e.what());
        return "";
    }
}

std::filesystem::path vk_test::pathFromUtf8(const char* utf8) noexcept {
    // Since this is the inverse of pathToUtf8, see pathToUtf8 for implementation
    // notes.
    try {
#ifdef _WIN32
        const size_t utf8_bytes = strlen(utf8);
        if (utf8_bytes == 0) {
            return L"";
        }
        if (utf8_bytes > std::numeric_limits<int>::max()) {
            VK_TEST_SAY(__func__ << " : Input had too many characters to store in an int.");
            return L"";
        }
        const int utf8_bytes_i     = static_cast<int>(utf8_bytes);
        const int utf16_characters = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, utf8_bytes_i, nullptr, 0);
        if (utf16_characters <= 0) {
            VK_TEST_SAY(__func__ << " : MultiByteToWideChar failed. The input is probably not valid UTF-8.");
            return L"";
        }
        static_assert(std::is_same_v<std::filesystem::path::string_type, std::wstring>);
        std::wstring result(utf16_characters, wchar_t(0));
        std::ignore = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, utf8_bytes_i, result.data(), utf16_characters);
        return std::filesystem::path(std::move(result));
#else
        return std::filesystem::path(utf8);
#endif
    }
    catch (const std::exception& e) {
        VK_TEST_SAY(__func__ << " threw an exception : " << e.what());
#ifdef _WIN32 // Avoid _convert_narrow_to_wide, just in case
        return L"";
#else
        return "";
#endif
    }
}

bool vk_test::extensionMatches(const std::filesystem::path& path, const char* extension) {
    // The standard implementation of this, tolower(path.extension()) == extension,
    // would use 3 allocations: path.extension(), a copy for tolower, and one for
    // fs::path(extension) (since == is only implemented for path == path).
    // We can bring this down to 1 on Windows and 0 on Linux with a custom implementation.

    // First, find the character where the extension starts.
    // Because we're just testing whether the extension matches, we don't need
    // to handle things like Windows' NTFS Alternate Data Streams.
    const std::filesystem::path::string_type&   native  = path.native();
    constexpr std::filesystem::path::value_type dot     = '.'; // Same value in UTF-8 and UTF-16
    const auto                                  dot_pos = native.rfind(dot);
    if (dot_pos == native.npos) // No extension?
    {
        return extension == nullptr || extension[0] == '\0';
    }

#ifdef _WIN32
    // UTF-8 to UTF-16 copy
    const std::filesystem::path              extension_ut_f16 = pathFromUtf8(extension);
    const std::filesystem::path::value_type* extension_bytes  = extension_ut_f16.native().c_str();
    return 0 == _wcsicmp(native.c_str() + dot_pos, extension_bytes);
#else
    return 0 == strcasecmp(native.c_str() + dotPos, extension);
#endif
}
