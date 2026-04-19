#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "ClothoidGen.h"
#include "../model/Road.h"

class BankAngleGen {
public:
    static std::vector<float> sampleAnglesRadians(
        const Road& road,
        const std::vector<CurvePt>& curve);
};
