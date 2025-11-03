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
#include "Slang.hpp"

vk_test::SlangCompiler::SlangCompiler() {
    SlangGlobalSessionDesc desc{ .enableGLSL = true };
    slang::createGlobalSession(&desc, m_GlobalSession.writeRef());
}

void vk_test::SlangCompiler::defaultTarget() {
    m_Targets.push_back({
        .format                      = SLANG_SPIRV,
        .profile                     = m_GlobalSession->findProfile("spirv_1_6+vulkan_1_4"),
        .flags                       = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY,
        .forceGLSLScalarBufferLayout = true,
    });
}

void vk_test::SlangCompiler::defaultOptions() {
    m_Options.push_back({ slang::CompilerOptionName::EmitSpirvDirectly, { slang::CompilerOptionValueKind::Int, 1 } });
    m_Options.push_back({ slang::CompilerOptionName::VulkanUseEntryPointName, { slang::CompilerOptionValueKind::Int, 1 } });
    m_Options.push_back({ slang::CompilerOptionName::AllowGLSL, { slang::CompilerOptionValueKind::Int, 1 } });
}

void vk_test::SlangCompiler::addSearchPaths(const std::vector<std::filesystem::path>& search_paths) {
    for (const auto& str : search_paths) {
        m_SearchPaths.push_back(str);                            // For vk_test::findFile()
        m_SearchPathsUtf8.push_back(vk_test::utf8FromPath(str)); // Need to keep the UTF-8 allocation alive
        // Slang expects const char* to UTF-8; see implementation of Slang's FileStream::_init().
        m_SearchPathsUtf8Pointers.push_back(m_SearchPathsUtf8.back().c_str());
    }
}

void vk_test::SlangCompiler::clearSearchPaths() {
    m_SearchPaths.clear();
    m_SearchPathsUtf8.clear();
    m_SearchPathsUtf8Pointers.clear();
}

const uint32_t* vk_test::SlangCompiler::getSpirv() const {
    if (m_Spirv == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<const uint32_t*>(m_Spirv->getBufferPointer());
}

size_t vk_test::SlangCompiler::getSpirvSize() const {
    if (m_Spirv == nullptr) {
        return 0;
    }
    return m_Spirv->getBufferSize();
}

slang::IComponentType* vk_test::SlangCompiler::getSlangProgram() const {
    if (m_LinkedProgram == nullptr) {
        return nullptr;
    }
    return m_LinkedProgram.get();
}

slang::IModule* vk_test::SlangCompiler::getSlangModule() const {
    if (m_Module == nullptr) {
        return nullptr;
    }
    return m_Module.get();
}

bool vk_test::SlangCompiler::compileFile(const std::filesystem::path& filename) {
    const std::filesystem::path source_file = vk_test::findFile(filename, m_SearchPaths);
    if (source_file.empty()) {
        m_LastDiagnosticMessage = "File not found : " + vk_test::utf8FromPath(filename);
        VK_TEST_SAY(m_LastDiagnosticMessage.c_str());
        return false;
    }
    bool success = loadFromSourceString(vk_test::utf8FromPath(source_file.stem()), vk_test::loadFile(source_file));
    if (success) {
        if (m_Callback) {
            m_Callback(source_file, getSpirv(), getSpirvSize());
        }
    }

    return success;
}

void vk_test::SlangCompiler::logAndAppendDiagnostics(slang::IBlob* diagnostics) {
    if (diagnostics != nullptr) {
        const char* message = reinterpret_cast<const char*>(diagnostics->getBufferPointer());
        // Since these are often multi-line, we want to print them with extra spaces:
        VK_TEST_SAY(message);
        // Append onto m_LastDiagnosticMessage, separated by a newline:
        if (m_LastDiagnosticMessage.empty()) {
            m_LastDiagnosticMessage += '\n';
        }
        m_LastDiagnosticMessage += message;
    }
}

bool vk_test::SlangCompiler::loadFromSourceString(const std::string& module_name, const std::string& slang_source) {
    createSession();

    // Clear any previous compilation
    m_Spirv = nullptr;
    m_LastDiagnosticMessage.clear();

    Slang::ComPtr<slang::IBlob> diagnostics;
    // From source code to Slang module
    m_Module = m_Session->loadModuleFromSourceString(module_name.c_str(), nullptr, slang_source.c_str(), diagnostics.writeRef());
    logAndAppendDiagnostics(diagnostics);
    if (m_Module == nullptr) {
        return false;
    }

    // In order to get entrypoint shader reflection, it seems like one must go
    // through the additional step of listing every entry point in the composite
    // type. This matches the docs, but @nbickford wonders if there's a simpler way.
    const SlangInt32                               defined_entry_point_count = m_Module->getDefinedEntryPointCount();
    std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points(defined_entry_point_count);
    std::vector<slang::IComponentType*>            components(1 + defined_entry_point_count);
    components[0] = m_Module;
    for (SlangInt32 i = 0; i < defined_entry_point_count; i++) {
        m_Module->getDefinedEntryPoint(i, entry_points[i].writeRef());
        components[1 + i] = entry_points[i];
    }

    Slang::ComPtr<slang::IComponentType> composed_program;
    SlangResult                          result = m_Session->createCompositeComponentType(components.data(), components.size(), composed_program.writeRef(), diagnostics.writeRef());
    logAndAppendDiagnostics(diagnostics);
    if (SLANG_FAILED(result) || (composed_program == nullptr)) {
        return false;
    }

    // From composite component type to linked program
    result = composed_program->link(m_LinkedProgram.writeRef(), diagnostics.writeRef());
    logAndAppendDiagnostics(diagnostics);
    if (SLANG_FAILED(result) || (m_LinkedProgram == nullptr)) {
        return false;
    }

    // From linked program to SPIR-V
    result = m_LinkedProgram->getTargetCode(0, m_Spirv.writeRef(), diagnostics.writeRef());
    logAndAppendDiagnostics(diagnostics);
    return !(SLANG_FAILED(result) || nullptr == m_Spirv);
}

void vk_test::SlangCompiler::createSession() {
    m_Session = {};

    slang::SessionDesc desc{
        .targets                  = m_Targets.data(),
        .targetCount              = SlangInt(m_Targets.size()),
        .searchPaths              = m_SearchPathsUtf8Pointers.data(),
        .searchPathCount          = SlangInt(m_SearchPathsUtf8Pointers.size()),
        .preprocessorMacros       = m_Macros.data(),
        .preprocessorMacroCount   = SlangInt(m_Macros.size()),
        .allowGLSLSyntax          = true,
        .compilerOptionEntries    = m_Options.data(),
        .compilerOptionEntryCount = uint32_t(m_Options.size()),
    };
    m_GlobalSession->createSession(desc, m_Session.writeRef());
}

//--------------------------------------------------------------------------------------------------
// Usage example
//--------------------------------------------------------------------------------------------------
static void usage_SlangCompiler() {
    vk_test::SlangCompiler slang_compiler;
    slang_compiler.defaultTarget();
    slang_compiler.defaultOptions();

    // Configure compiler settings as you wish
    const std::vector<std::filesystem::path> shaders_paths = { "include/shaders" };
    slang_compiler.addSearchPaths(shaders_paths);
    slang_compiler.addOption({ slang::CompilerOptionName::DebugInformation,
                              { slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_MAXIMAL } });
    slang_compiler.addMacro({ "MY_DEFINE", "1" });

    // Compile a shader file
    bool success = slang_compiler.compileFile("shader.slang");

    // Check if compilation was successful
    if (!success) {
        // Get the error message
        const std::string& error_messages = slang_compiler.getLastDiagnosticMessage();
        VK_TEST_SAY("Compilation failed : " << error_messages.c_str());
    }
    else {
        // Get the compiled SPIR-V code
        const uint32_t* spirv     = slang_compiler.getSpirv();
        size_t          spirv_size = slang_compiler.getSpirvSize();

        // Check if there were any warnings
        const std::string& warning_messages = slang_compiler.getLastDiagnosticMessage();
        if (!warning_messages.empty()) {
            VK_TEST_SAY("Compilation succeeded with warnings : " << warning_messages.c_str());
        }
    }
}
