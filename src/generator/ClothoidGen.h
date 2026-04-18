#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../model/Road.h"

namespace ClothoidGen {

// Build a smooth centerline for the given control points using
// the clothoid (Euler spiral) curve method.
// sampleInterval: approximate world-space distance between output points.
std::vector<glm::vec3> buildCenterline(
    const std::vector<ControlPoint>& pts,
    float sampleInterval = 1.0f);

// Resample a polyline at equal arc-length intervals.
// Returns points spaced exactly `interval` apart, preserving start/end.
std::vector<glm::vec3> resamplePolyline(
    const std::vector<glm::vec3>& pts,
    float interval);

// Convenience: build clothoid centerline then resample at equal intervals.
// Use a fine internal sampleInterval (interval/10) for accuracy.
std::vector<glm::vec3> buildAndResample(
    const std::vector<ControlPoint>& pts,
    float interval);

} // namespace ClothoidGen
