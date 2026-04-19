#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../model/Road.h"
#include "ClothoidGen.h"

namespace VerticalCurveGen {

std::vector<glm::vec3> apply(
    const Road& road,
    const std::vector<glm::vec3>& baseCurve,
    float sampleInterval);

std::vector<CurvePt> applyDetailed(
    const Road& road,
    const std::vector<CurvePt>& baseCurve,
    float sampleInterval);

} // namespace VerticalCurveGen
