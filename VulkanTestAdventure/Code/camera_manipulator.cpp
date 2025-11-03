/*
 * Copyright (c) 2018-2025, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2018-2025, NVIDIA CORPORATION.
 * SPDX-License-Identifier: Apache-2.0
 */
//--------------------------------------------------------------------

/*
* Modified by Farrakh
* 2025
*/

#include "pch.h"
#include "camera_manipulator.hpp"

#ifdef _MSC_VER
#define VK_TEST_SAFE_SSCANF sscanf_s
#else
#define SAFE_SSCANF sscanf
#endif

namespace vk_test {

    //--------------------------------------------------------------------------------------------------
    CameraManipulator::CameraManipulator() {
        updateLookatMatrix();
    }

    //--------------------------------------------------------------------------------------------------
    // Set the new camera as a goal
    // instantSet = true will not interpolate to the new position
    void CameraManipulator::setCamera(Camera camera, bool instant_set /*=true*/) {
        m_AnimDone = true;

        if (instant_set || m_Duration == 0.0) {
            m_Current = camera;
            updateLookatMatrix();
        }
        else if (camera != m_Current) {
            m_Goal      = camera;
            m_Snapshot  = m_Current;
            m_AnimDone  = false;
            m_StartTime = getSystemTime();
            findBezierPoints();
        }
    }

    //--------------------------------------------------------------------------------------------------
    // Creates a viewing matrix derived from an eye point, a reference point indicating the center of
    // the scene, and an up vector
    //
    void CameraManipulator::setLookat(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool instant_set) {
        setCamera({ eye, center, up, m_Current.fov, m_Current.clip }, instant_set);
    }

    //-----------------------------------------------------------------------------
    // Get the current camera's look-at parameters.
    void CameraManipulator::getLookat(glm::vec3& eye, glm::vec3& center, glm::vec3& up) const {
        eye    = m_Current.eye;
        center = m_Current.ctr;
        up     = m_Current.up;
    }

    //--------------------------------------------------------------------------------------------------
    // Pan the camera perpendicularly to the light of sight.
    void CameraManipulator::pan(glm::vec2 displacement) {
        if (m_Mode == Fly) {
            displacement *= -1.F;
        }

        glm::vec3 view_direction(m_Current.eye - m_Current.ctr);
        float     view_distance = static_cast<float>(glm::length(view_direction)) / 0.785F; // 45 degrees
        view_direction          = glm::normalize(view_direction);
        glm::vec3 right_vector  = glm::cross(m_Current.up, view_direction);
        glm::vec3 up_vector     = glm::cross(view_direction, right_vector);
        right_vector            = glm::normalize(right_vector);
        up_vector               = glm::normalize(up_vector);

        glm::vec3 pan_offset = (-displacement.x * right_vector + displacement.y * up_vector) * view_distance;
        m_Current.eye += pan_offset;
        m_Current.ctr += pan_offset;
    }

    //--------------------------------------------------------------------------------------------------
    // Orbit the camera around the center of interest. If 'invert' is true,
    // then the camera stays in place and the interest orbit around the camera.
    //
    void CameraManipulator::orbit(glm::vec2 displacement, bool invert /*= false*/) {
        if (displacement == glm::vec2(0.F, 0.F)) {
            return;
}

        // Full width will do a full turn
        displacement *= glm::two_pi<float>();

        // Get the camera
        glm::vec3 origin(invert ? m_Current.eye : m_Current.ctr);
        glm::vec3 position(invert ? m_Current.ctr : m_Current.eye);

        // Get the length of sight
        glm::vec3 center_to_eye(position - origin);
        float     radius = glm::length(center_to_eye);
        center_to_eye      = glm::normalize(center_to_eye);
        glm::vec3 axe_z   = center_to_eye;

        // Find the rotation around the UP axis (Y)
        glm::mat4 rot_y = glm::rotate(glm::mat4(1), -displacement.x, m_Current.up);

        // Apply the (Y) rotation to the eye-center vector
        center_to_eye = rot_y * glm::vec4(center_to_eye, 0);

        // Find the rotation around the X vector: cross between eye-center and up (X)
        glm::vec3 axe_x = glm::normalize(glm::cross(m_Current.up, axe_z));
        glm::mat4 rot_x = glm::rotate(glm::mat4(1), -displacement.y, axe_x);

        // Apply the (X) rotation to the eye-center vector
        glm::vec3 rotation_vec = rot_x * glm::vec4(center_to_eye, 0);

        if (glm::sign(rotation_vec.x) == glm::sign(center_to_eye.x)) {
            center_to_eye = rotation_vec;
}

        // Make the vector as long as it was originally
        center_to_eye *= radius;

        // Finding the new position
        glm::vec3 new_position = center_to_eye + origin;

        if (!invert) {
            m_Current.eye = new_position; // Normal: change the position of the camera
        }
        else {
            m_Current.ctr = new_position; // Inverted: change the interest point
        }
    }

    //--------------------------------------------------------------------------------------------------
    // Move the camera toward the interest point, but don't cross it
    //
    void CameraManipulator::dolly(glm::vec2 displacement, bool keep_center_fixed /*= false*/) {
        glm::vec3 direction_vec = m_Current.ctr - m_Current.eye;
        float     length       = static_cast<float>(glm::length(direction_vec));

        // We are at the point of interest, do nothing!
        if (length < 0.000001F) {
            return;
}

        // Use the larger movement.
        float larger_displacement = fabs(displacement.x) > fabs(displacement.y) ? displacement.x : -displacement.y;

        // Don't move over the point of interest.
        if (larger_displacement >= 1.0F) {
            return;
}

        direction_vec *= larger_displacement;

        // Not going up
        if (m_Mode == Walk) {
            if (m_Current.up.y > m_Current.up.z) {
                direction_vec.y = 0;
            } else {
                direction_vec.z = 0;
}
        }

        m_Current.eye += direction_vec;

        // In fly mode, the interest moves with us.
        if ((m_Mode == Fly || m_Mode == Walk) && !keep_center_fixed) {
            m_Current.ctr += direction_vec;
        }
    }

    //--------------------------------------------------------------------------------------------------
    // Modify the position of the camera over time
    // - The camera can be updated through keys. A key set a direction which is added to both
    //   eye and center, until the key is released
    // - A new position of the camera is defined and the camera will reach that position
    //   over time.
    void CameraManipulator::updateAnim() {
        auto elapse = static_cast<float>(getSystemTime() - m_StartTime) / 1000.F;

        // Camera moving to new position
        if (m_AnimDone) {
            return;
}

        float t = std::min(elapse / float(m_Duration), 1.0F);
        // Evaluate polynomial (smoother step from Perlin)
        t = t * t * t * (t * (t * 6.0F - 15.0F) + 10.0F);
        if (t >= 1.0F) {
            m_Current  = m_Goal;
            m_AnimDone = true;
            updateLookatMatrix();
            return;
        }

        // Interpolate camera position and interest
        // The distance of the camera between the interest is preserved to create a nicer interpolation
        m_Current.ctr  = glm::mix(m_Snapshot.ctr, m_Goal.ctr, t);
        m_Current.up   = glm::mix(m_Snapshot.up, m_Goal.up, t);
        m_Current.eye  = computeBezier(t, m_Bezier[0], m_Bezier[1], m_Bezier[2]);
        m_Current.fov  = glm::mix(m_Snapshot.fov, m_Goal.fov, t);
        m_Current.clip = glm::mix(m_Snapshot.clip, m_Goal.clip, t);

        updateLookatMatrix();
    }

    //--------------------------------------------------------------------------------------------------
    //
    void CameraManipulator::setMatrix(const glm::mat4& matrix, bool instant_set, float center_distance) {
        Camera camera;
        camera.eye = matrix[3];

        auto rot_mat = glm::mat3(matrix);
        camera.ctr  = { 0, 0, -center_distance };
        camera.ctr  = camera.eye + (rot_mat * camera.ctr);
        camera.up   = { 0, 1, 0 };
        camera.fov  = m_Current.fov;

        m_AnimDone = instant_set;

        if (instant_set) {
            m_Current = camera;
        }
        else {
            m_Goal      = camera;
            m_Snapshot  = m_Current;
            m_StartTime = getSystemTime();
            findBezierPoints();
        }
        updateLookatMatrix();
    }

    //--------------------------------------------------------------------------------------------------
    // Low level function for when the camera move.
    void CameraManipulator::motion(const glm::vec2& screen_displacement, Actions action /*= 0*/) {
        glm::vec2 displacement = {
            float(screen_displacement.x - m_Mouse[0]) / float(m_WindowSize.x),
            float(screen_displacement.y - m_Mouse[1]) / float(m_WindowSize.y),
        };

        switch (action) {
            case CameraManipulator::Orbit:
                orbit(displacement, false);
                break;
            case CameraManipulator::Dolly:
                dolly(displacement);
                break;
            case CameraManipulator::Pan:
                pan(displacement);
                break;
            case CameraManipulator::LookAround:
                orbit({ displacement.x, -displacement.y }, true);
                break;
            default:
                break;
        }

        // Resetting animation and update the camera
        m_AnimDone = true;
        updateLookatMatrix();

        m_Mouse = screen_displacement;
    }

    //--------------------------------------------------------------------------------------------------
    // Function for when the camera move with keys (ex. WASD).
    // Note: dx and dy are the speed of the camera movement.
    void CameraManipulator::keyMotion(glm::vec2 delta, Actions action) {
        float movement_speed = m_Speed;

        auto direction_vector = glm::normalize(m_Current.ctr - m_Current.eye); // Vector from eye to center
        delta *= movement_speed;

        glm::vec3 keyboard_movement_vector{ 0, 0, 0 };
        if (action == Dolly) {
            keyboard_movement_vector = direction_vector * delta.x;
            if (m_Mode == Walk) {
                if (m_Current.up.y > m_Current.up.z) {
                    keyboard_movement_vector.y = 0;
                } else {
                    keyboard_movement_vector.z = 0;
}
            }
        }
        else if (action == Pan) {
            auto right_vector       = glm::cross(direction_vector, m_Current.up);
            keyboard_movement_vector = right_vector * delta.x + m_Current.up * delta.y;
        }

        m_Current.eye += keyboard_movement_vector;
        m_Current.ctr += keyboard_movement_vector;

        // Resetting animation and update the camera
        m_AnimDone = true;
        updateLookatMatrix();
    }

    //--------------------------------------------------------------------------------------------------
    // To call when the mouse is moving
    // It find the appropriate camera operator, based on the mouse button pressed and the
    // keyboard modifiers (shift, ctrl, alt)
    //
    // Returns the action that was activated
    //
    CameraManipulator::Actions CameraManipulator::mouseMove(glm::vec2 screen_displacement, const Inputs& inputs) {
        if (!inputs.lmb && !inputs.rmb && !inputs.mmb) {
            setMousePosition(screen_displacement);
            return NoAction; // no mouse button pressed
        }

        Actions cur_action = NoAction;
        if (inputs.lmb) {
            if (((inputs.ctrl) && (inputs.shift)) || inputs.alt) {
                cur_action = m_Mode == Examine ? LookAround : Orbit;
            } else if (inputs.shift) {
                cur_action = Dolly;
            } else if (inputs.ctrl) {
                cur_action = Pan;
            } else {
                cur_action = m_Mode == Examine ? Orbit : LookAround;
}
        }
        else if (inputs.mmb) {
            cur_action = Pan;
        } else if (inputs.rmb) {
            cur_action = Dolly;
}

        if (cur_action != NoAction) {
            motion(screen_displacement, cur_action);
}

        return cur_action;
    }

    //--------------------------------------------------------------------------------------------------
    // Trigger a dolly when the wheel change, or change the FOV if the shift key was pressed
    //
    void CameraManipulator::wheel(float value, const Inputs& inputs) {
        float delta_x = (value * fabsf(value)) / static_cast<float>(m_WindowSize.x);

        if (inputs.shift) {
            setFov(m_Current.fov + value);
        }
        else {
            // Dolly in or out. CTRL key keeps center fixed, which has for side effect to adjust the speed for fly/walk mode
            dolly(glm::vec2(delta_x), inputs.ctrl);
            updateLookatMatrix();
        }
    }

    // Set and clamp FOV between 0.01 and 179 degrees
    void CameraManipulator::setFov(float fov_degree) {
        m_Current.fov = std::min(std::max(fov_degree, 0.01F), 179.0F);
    }

    glm::vec3 CameraManipulator::computeBezier(float t, glm::vec3& p0, glm::vec3& p1, glm::vec3& p2) {
        float u  = 1.F - t;
        float tt = t * t;
        float uu = u * u;

        glm::vec3 p = uu * p0; // first term
        p += 2 * u * t * p1;   // second term
        p += tt * p2;          // third term

        return p;
    }

    void CameraManipulator::findBezierPoints() {
        glm::vec3 p0 = m_Current.eye;
        glm::vec3 p2 = m_Goal.eye;
        glm::vec3 p1;
        glm::vec3 pc;

        // point of interest
        glm::vec3 pi = (m_Goal.ctr + m_Current.ctr) * 0.5F;

        glm::vec3 p02    = (p0 + p2) * 0.5F;                           // mid p0-p2
        float     radius = (length(p0 - pi) + length(p2 - pi)) * 0.5F; // Radius for p1
        glm::vec3 p02pi(p02 - pi);                                     // Vector from interest to mid point
        p02pi = glm::normalize(p02pi);
        p02pi *= radius;
        pc   = pi + p02pi;                       // Calculated point to go through
        p1   = 2.F * pc - p0 * 0.5F - p2 * 0.5F; // Computing p1 for t=0.5
        p1.y = p02.y;                            // Clamping the P1 to be in the same height as p0-p2

        m_Bezier[0] = p0;
        m_Bezier[1] = p1;
        m_Bezier[2] = p2;
    }

    //--------------------------------------------------------------------------------------------------
    // Return the time in fraction of milliseconds
    //
    double CameraManipulator::getSystemTime() {
        auto now(std::chrono::system_clock::now());
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000.0;
    }

    //--------------------------------------------------------------------------------------------------
    // Return a string which can be included in help dialogs
    //
    const std::string& CameraManipulator::getHelp() {
        static std::string help_text =
            "LMB: rotate around the target\n"
            "RMB: Dolly in/out\n"
            "MMB: Pan along view plane\n"
            "LMB + Shift: Dolly in/out\n"
            "LMB + Ctrl: Pan\n"
            "LMB + Alt: Look aroundPan\n"
            "Mouse wheel: Dolly in/out\n"
            "Mouse wheel + Shift: Zoom in/out\n";
        return help_text;
    }

    //--------------------------------------------------------------------------------------------------
    // Move the camera closer or further from the center of the the bounding box, to see it completely
    //
    // boxMin - lower corner of the bounding box
    // boxMax - upper corner of the bounding box
    // instantFit - true: set the new position, false: will animate to new position.
    // tight - true: fit exactly the corner, false: fit to radius (larger view, will not get closer or further away)
    // aspect - aspect ratio of the window.
    //
    void CameraManipulator::fit(const glm::vec3& box_min, const glm::vec3& box_max, bool instant_fit /*= true*/, bool tight_fit /*=false*/, float aspect /*=1.0f*/) {
        // Calculate the half extents of the bounding box
        const glm::vec3 box_half_size = 0.5F * (box_max - box_min);

        // Calculate the center of the bounding box
        const glm::vec3 box_center = 0.5F * (box_min + box_max);

        const float yfov = tan(glm::radians(m_Current.fov * 0.5F));
        const float xfov = yfov * aspect;

        // Calculate the ideal distance for a tight fit or fit to radius
        float ideal_distance = 0;

        if (tight_fit) {
            // Get only the rotation matrix
            glm::mat3 m_view = glm::lookAt(m_Current.eye, box_center, m_Current.up);

            // Check each 8 corner of the cube
            for (int i = 0; i < 8; i++) {
                // Rotate the bounding box in the camera view
                glm::vec3 vct(((i & 1) != 0) ? box_half_size.x : -box_half_size.x,  //
                              ((i & 2) != 0) ? box_half_size.y : -box_half_size.y,  //
                              ((i & 4) != 0) ? box_half_size.z : -box_half_size.z); //
                vct = m_view * vct;

                if (vct.z < 0) // Take only points in front of the center
                {
                    // Keep the largest offset to see that vertex
                    ideal_distance = std::max((fabsf(vct.y) / yfov) + fabsf(vct.z), ideal_distance);
                    ideal_distance = std::max((fabsf(vct.x) / xfov) + fabsf(vct.z), ideal_distance);
                }
            }
        }
        else // Using the bounding sphere
        {
            const float radius = glm::length(box_half_size);
            ideal_distance      = std::max(radius / xfov, radius / yfov);
        }

        // Calculate the new camera position based on the ideal distance
        const glm::vec3 new_eye = box_center - ideal_distance * glm::normalize(box_center - m_Current.eye);

        // Set the new camera position and interest point
        setLookat(new_eye, box_center, m_Current.up, instant_fit);
    }

    std::string CameraManipulator::Camera::getString() const {
        return std::format("{{{}, {}, {}}}, {{{}, {}, {}}}, {{{}, {}, {}}}, {{{}}}, {{{}, {}}}", //
                           eye.x,
                           eye.y,
                           eye.z, //
                           ctr.x,
                           ctr.y,
                           ctr.z, //
                           up.x,
                           up.y,
                           up.z, //
                           fov,
                           clip.x,
                           clip.y);
    }

    bool CameraManipulator::Camera::setFromString(const std::string& text) {
        if (text.empty()) { return false;
}

        std::array<float, 12> val{};
        int                   result = VK_TEST_SAFE_SSCANF(text.c_str(), "{%f, %f, %f}, {%f, %f, %f}, {%f, %f, %f}, {%f}, {%f, %f}", val.data(), &val[1], &val[2], &val[3], &val[4], &val[5], &val[6], &val[7], &val[8], &val[9], &val[10], &val[11]);
        if (result >= 9) // Before 2025-09-03, this format didn't include the FOV at the end
        {
            eye = glm::vec3{ val[0], val[1], val[2] };
            ctr = glm::vec3{ val[3], val[4], val[5] };
            up  = glm::vec3{ val[6], val[7], val[8] };
            if (result >= 10) {
                fov = val[9];
}
            if (result >= 11) {
                clip = glm::vec2{ val[10], val[11] };
}

            return true;
        }
        return false;
    }

} // namespace vk_test
