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

#pragma once
#include <array>
#include <vector>
#include <string>
#include <filesystem>

#pragma push_macro("None")
#pragma push_macro("Bool")
#undef None
#undef Bool
#include <slang/slang.h>
#pragma pop_macro("None")
#pragma pop_macro("Bool")

#include <slang/slang-com-ptr.h>
#include <file_operations.hpp>
#include "spirv.hpp"

//--------------------------------------------------------------------------------------------------
// Slang Compiler
//
// Usage:
//   see usage_SlangCompiler
//--------------------------------------------------------------------------------------------------

namespace vk_test {
    // A class responsible for compiling Slang source code.
    class SlangCompiler {
    public:
        SlangCompiler();
        ~SlangCompiler() = default;

        void defaultTarget();  // Default target is SPIR-V
        void defaultOptions(); // Default options are EmitSpirvDirectly, VulkanUseEntryPointName

        void                                     addOption(const slang::CompilerOptionEntry& option) { m_Options.push_back(option); }
        void                                     clearOptions() { m_Options.clear(); }
        std::vector<slang::CompilerOptionEntry>& options() { return m_Options; }

        void                            addTarget(const slang::TargetDesc& target) { m_Targets.push_back(target); }
        void                            clearTargets() { m_Targets.clear(); }
        std::vector<slang::TargetDesc>& targets() { return m_Targets; }

        void addSearchPaths(const std::vector<std::filesystem::path>& search_paths);
        void clearSearchPaths();
        // This is const because modifiying the search paths requires extra work.
        const std::vector<std::filesystem::path>& searchPaths() const { return m_SearchPaths; }

        void                                       addMacro(const slang::PreprocessorMacroDesc& macro) { m_Macros.push_back(macro); }
        void                                       clearMacros() { m_Macros.clear(); }
        std::vector<slang::PreprocessorMacroDesc>& macros() { return m_Macros; }

        // Compile a file or source
        bool compileFile(const std::filesystem::path& filename);
        bool loadFromSourceString(const std::string& module_name, const std::string& slang_source);

        // Get result of the compilation
        const uint32_t* getSpirv() const;
        // Get the number of bytes in the compiled SPIR-V.
        size_t getSpirvSize() const;
        // Gets the linked Slang program; does not add a reference to it.
        // This is usually what you want for reflection.
        slang::IComponentType* getSlangProgram() const;
        // Gets the Slang module; does not add a reference to it. This is usually
        // useful for reflection if you need a list of functions.
        slang::IModule* getSlangModule() const;

        // Use for Dump or Aftermath
        void setCompileCallback(std::function<void(const std::filesystem::path& source_file, const uint32_t* spirv_code, size_t spirv_size)> callback) {
            m_Callback = callback;
        }

        // Get the last diagnostic message (error or warning).
        // Multiple diagnostics are each separated by a single newline.
        const std::string& getLastDiagnosticMessage() const { return m_LastDiagnosticMessage; }

    private:
        void createSession();
        void logAndAppendDiagnostics(slang::IBlob* diagnostic);

        Slang::ComPtr<slang::IGlobalSession>      m_GlobalSession;
        std::vector<slang::TargetDesc>            m_Targets;
        std::vector<slang::CompilerOptionEntry>   m_Options;
        std::vector<std::filesystem::path>        m_SearchPaths;
        std::vector<std::string>                  m_SearchPathsUtf8;
        std::vector<const char*>                  m_SearchPathsUtf8Pointers;
        Slang::ComPtr<slang::ISession>            m_Session;
        Slang::ComPtr<slang::IModule>             m_Module;
        Slang::ComPtr<slang::IComponentType>      m_LinkedProgram;
        Slang::ComPtr<ISlangBlob>                 m_Spirv;
        std::vector<slang::PreprocessorMacroDesc> m_Macros;

        std::function<void(const std::filesystem::path& source_file, const uint32_t* spirv_code, size_t spirv_size)> m_Callback;

        // Store the last diagnostic message
        std::string m_LastDiagnosticMessage;
    };

} // namespace vk_test
