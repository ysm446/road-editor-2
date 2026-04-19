#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "ClothoidGen.h"
#include "../model/Road.h"

struct CurveFrame {
    glm::vec3 tangent = {0.0f, 0.0f, 1.0f};
    glm::vec3 right   = {1.0f, 0.0f, 0.0f};
    glm::vec3 up      = {0.0f, 1.0f, 0.0f};
    float angleRadians = 0.0f;
};

class BankAngleGen {
public:
    static std::vector<float> sampleAnglesRadians(
        const Road& road,
        const std::vector<CurvePt>& curve);

    static std::vector<CurveFrame> sampleFrames(
        const std::vector<CurvePt>& curve,
        const std::vector<float>& anglesRadians);
};
