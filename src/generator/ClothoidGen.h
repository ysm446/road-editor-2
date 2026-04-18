#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../model/Road.h"

// Segment type tag for each point in the detailed centerline.
// The kind describes the curve type of the segment STARTING at that point.
enum class SegKind { Straight, Clothoid, Arc };

struct CurvePt {
    glm::vec3 pos;
    SegKind   kind = SegKind::Straight;
};

namespace ClothoidGen {

// Build a smooth centerline for the given control points using
// the clothoid (Euler spiral) curve method.
// sampleInterval: approximate world-space distance between output points.
// equalMidpoint: if true the bend midpoints are at t=0.5 on each edge;
//                if false (default) they are weighted by adjacent edge lengths.
std::vector<glm::vec3> buildCenterline(
    const std::vector<ControlPoint>& pts,
    float sampleInterval = 1.0f,
    bool  equalMidpoint  = false);

// Same as buildCenterline but each point is tagged with its segment type
// (Straight / Clothoid / Arc) for visualization.
std::vector<CurvePt> buildCenterlineDetailed(
    const std::vector<ControlPoint>& pts,
    float sampleInterval = 1.0f,
    bool  equalMidpoint  = false);

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
