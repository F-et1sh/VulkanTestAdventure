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
*/

#include "pch.h"
#include "element_default_title.hpp"

#include <GLFW/glfw3.h>
#undef APIENTRY

#include <file_operations.hpp>

vk_test::ElementDefaultWindowTitle::ElementDefaultWindowTitle(const std::string& prefix /*= ""*/, const std::string& suffix /*= ""*/)
    : m_Prefix(prefix), m_Suffix(suffix) {
}

void vk_test::ElementDefaultWindowTitle::onAttach(vk_test::Application* app) {
    VK_TEST_SAY("Adding DefaultWindowTitle\n");
    m_App = app;
}

void vk_test::ElementDefaultWindowTitle::onUIRender() {
    //GLFWwindow* window = m_App->getWindowHandle();
    //
    //// This can happen in headless mode
    //if (window == nullptr) return;

    //// Window Title
    //m_DirtyTimer += ImGui::GetIO().DeltaTime;
    //if (m_DirtyTimer > 1.0F) // Refresh every second
    //{
    //    const auto& size = m_App->getViewportSize();
    //    std::string title;
    //    if (!m_Prefix.empty()) {
    //        title += std::format("{} | ", m_Prefix.c_str());
    //    }
    //    const std::string exeName = vk_test::utf8FromPath(vk_test::getExecutablePath().stem());
    //    title += std::format("{} | {}x{} | {:.0f} FPS / {:.3f}ms", exeName, size.width, size.height, ImGui::GetIO().Framerate, 1000.F / ImGui::GetIO().Framerate);
    //    if (!m_Suffix.empty()) {
    //        title += std::format(" | {}", m_Suffix.c_str());
    //    }
    //    glfwSetWindowTitle(m_App->getWindowHandle(), title.c_str());
    //    m_DirtyTimer = 0;
    //}
}

void vk_test::ElementDefaultWindowTitle::setPrefix(const std::string& str) {
    m_Prefix = str;
}

void vk_test::ElementDefaultWindowTitle::setSuffix(const std::string& str) {
    m_Suffix = str;
}
