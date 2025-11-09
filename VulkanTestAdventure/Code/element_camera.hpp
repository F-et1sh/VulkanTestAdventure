/*
 * Copyright (c) 2022-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2022-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
* Modified by Farrakh
* 2025
*/

#pragma once

#include <camera_manipulator.hpp>

#include "application.hpp"

/*-------------------------------------------------------------------------------------------------
# class nvvkhl::ElementCamera

This class is an element of the application that is responsible for the camera manipulation. 
It is using the `CameraManipulator` to handle the camera movement and interaction.

To use this class, you need to add it to the `nvvkhl::Application` using the `addElement` method.

-------------------------------------------------------------------------------------------------*/

namespace vk_test {

    struct ElementCamera : public IAppElement {
        ElementCamera(std::shared_ptr<CameraManipulator> camera = nullptr) { m_CameraManipulator = camera; }

        void setCameraManipulator(std::shared_ptr<CameraManipulator>& pCamera) { m_CameraManipulator = pCamera; }
        void onAttach(Application* app) override;
        void onUIRender() override;
        void onResize(VkCommandBuffer cmd, const VkExtent2D& size) override;

        std::shared_ptr<CameraManipulator> getCameraManipulator() const { return m_CameraManipulator; }

        // Can be called independently
        //static void updateCamera(std::shared_ptr<CameraManipulator> m_CameraManipulator, ImGuiWindow* viewportWindow);

    private:
        std::shared_ptr<CameraManipulator> m_CameraManipulator{};
    };

} // namespace vk_test
