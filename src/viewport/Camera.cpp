#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

Camera::Camera() {}

glm::vec3 Camera::position() const {
    return m_target + glm::vec3(
        m_distance * std::cos(m_pitch) * std::sin(m_yaw),
        m_distance * std::sin(m_pitch),
        m_distance * std::cos(m_pitch) * std::cos(m_yaw)
    );
}

CameraState Camera::state() const {
    CameraState state;
    state.target = m_target;
    state.distance = m_distance;
    state.yaw = m_yaw;
    state.pitch = m_pitch;
    state.fovDeg = m_fovDeg;
    return state;
}

void Camera::applyState(const CameraState& state) {
    m_target = state.target;
    m_distance = std::clamp(state.distance, 1.0f, 4000.0f);
    m_yaw = state.yaw;
    m_pitch = std::clamp(state.pitch, -1.4f, 1.4f);
    m_fovDeg = std::clamp(state.fovDeg, 10.0f, 120.0f);
}

void Camera::setTarget(const glm::vec3& target) {
    m_target = target;
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projMatrix(float aspect) const {
    return glm::perspective(glm::radians(m_fovDeg), aspect, m_near, m_far);
}

void Camera::orbit(float dx, float dy) {
    m_yaw   -= dx * 0.005f;
    m_pitch += dy * 0.005f;
    m_pitch  = std::clamp(m_pitch, -1.4f, 1.4f);
}

void Camera::pan(float dx, float dy) {
    glm::vec3 eye     = position();
    glm::vec3 forward = glm::normalize(m_target - eye);
    glm::vec3 right   = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up      = glm::cross(right, forward);
    float     speed   = m_distance * 0.001f;
    m_target -= right * dx * speed;
    m_target += up    * dy * speed;
}

void Camera::zoom(float delta) {
    m_distance -= delta * m_distance * 0.1f;
    m_distance  = std::clamp(m_distance, 1.0f, 4000.0f);
}
