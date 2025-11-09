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
#include "element_camera.hpp"

//void vk_test::ElementCamera::updateCamera(std::shared_ptr<vk_test::CameraManipulator> m_cameraManip, ImGuiWindow* viewportWindow) {
//    vk_test::CameraManipulator::Inputs inputs; // Mouse and keyboard inputs
//
//    m_cameraManip->updateAnim(); // This makes the camera to transition smoothly to the new position
//
//    // Check if the mouse cursor is over the "Viewport", check for all inputs that can manipulate the camera.
//    if (!nvgui::isWindowHovered(viewportWindow))
//        return;
//
//    inputs.lmb      = ImGui::IsMouseDown(ImGuiMouseButton_Left);
//    inputs.rmb      = ImGui::IsMouseDown(ImGuiMouseButton_Right);
//    inputs.mmb      = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
//    inputs.ctrl     = ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl);
//    inputs.shift    = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);
//    inputs.alt      = ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);
//    ImVec2 mousePos = ImGui::GetMousePos();
//
//    // None of the modifiers should be pressed for the single key: WASD and arrows
//    if (!inputs.alt) {
//        // Speed of the camera movement when using WASD and arrows
//        float keyMotionFactor = ImGui::GetIO().DeltaTime;
//        if (inputs.shift) {
//            keyMotionFactor *= 5.0F; // Speed up the camera movement
//        }
//        if (inputs.ctrl) {
//            keyMotionFactor *= 0.1F; // Slow down the camera movement
//        }
//
//        if (ImGui::IsKeyDown(ImGuiKey_W)) {
//            m_cameraManip->keyMotion({ keyMotionFactor, 0 }, vk_test::CameraManipulator::Dolly);
//            inputs.shift = inputs.ctrl = false;
//        }
//
//        if (ImGui::IsKeyDown(ImGuiKey_S)) {
//            m_cameraManip->keyMotion({ -keyMotionFactor, 0 }, vk_test::CameraManipulator::Dolly);
//            inputs.shift = inputs.ctrl = false;
//        }
//
//        if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
//            m_cameraManip->keyMotion({ keyMotionFactor, 0 }, vk_test::CameraManipulator::Pan);
//            inputs.shift = inputs.ctrl = false;
//        }
//
//        if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
//            m_cameraManip->keyMotion({ -keyMotionFactor, 0 }, vk_test::CameraManipulator::Pan);
//            inputs.shift = inputs.ctrl = false;
//        }
//
//        if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
//            m_cameraManip->keyMotion({ 0, keyMotionFactor }, vk_test::CameraManipulator::Pan);
//            inputs.shift = inputs.ctrl = false;
//        }
//
//        if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
//            m_cameraManip->keyMotion({ 0, -keyMotionFactor }, vk_test::CameraManipulator::Pan);
//            inputs.shift = inputs.ctrl = false;
//        }
//    }
//
//    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
//        m_cameraManip->setMousePosition({ mousePos.x, mousePos.y });
//    }
//
//    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 1.0F) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 1.0F) || ImGui::IsMouseDragging(ImGuiMouseButton_Right, 1.0F)) {
//        m_cameraManip->mouseMove({ mousePos.x, mousePos.y }, inputs);
//    }
//
//    // Mouse Wheel
//    if (ImGui::GetIO().MouseWheel != 0.0F) {
//        m_cameraManip->wheel(ImGui::GetIO().MouseWheel * -3.f, inputs);
//    }
//}

void vk_test::ElementCamera::onAttach(vk_test::Application* app) {
    VK_TEST_SAY("Adding Camera Manipulator");
}

void vk_test::ElementCamera::onUIRender() {
    assert(m_CameraManipulator && "Missing setCamera");
    //updateCamera(m_CameraManipulator, ImGui::FindWindowByName("Viewport"));
}

void vk_test::ElementCamera::onResize(VkCommandBuffer cmd, const VkExtent2D& size) {
    assert(m_CameraManipulator && "Missing setCamera");
    m_CameraManipulator->setWindowSize({ size.width, size.height });
}
