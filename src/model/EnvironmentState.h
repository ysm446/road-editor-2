#pragma once

#include <glm/glm.hpp>
#include "RoadNetwork.h"

struct CameraState {
    glm::vec3 target = {0.0f, 0.0f, 0.0f};
    float     distance = 400.0f;
    float     yaw = 0.5f;
    float     pitch = 0.5f;
    float     fovDeg = 45.0f;
};

struct EnvironmentState {
    int           version = 1;
    TerrainSettings terrain;
    CameraState   camera;
};
