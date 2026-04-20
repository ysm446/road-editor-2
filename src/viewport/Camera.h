#pragma once

#include <glm/glm.hpp>
#include "../model/EnvironmentState.h"

// Orbit camera (Maya-style: Alt+LMB rotate, Alt+MMB pan, Alt+wheel zoom).
// World space: right-handed, Y-up, X+ right.
class Camera {
public:
    Camera();

    glm::mat4 viewMatrix() const;
    glm::mat4 projMatrix(float aspect) const;
    glm::vec3 position() const;
    CameraState state() const;
    void applyState(const CameraState& state);
    void setTarget(const glm::vec3& target);

    void orbit(float dx, float dy); // pixels
    void pan(float dx, float dy);   // pixels
    void zoom(float delta);         // wheel notches

private:
    glm::vec3 m_target   = {0.0f, 0.0f, 0.0f};
    float     m_distance = 400.0f;
    float     m_yaw      = 0.5f;   // radians, rotation around Y
    float     m_pitch    = 0.5f;   // radians, elevation
    float     m_fovDeg   = 45.0f;
    float     m_near     = 0.5f;
    float     m_far      = 5000.0f;
};
