#include "BankAngleGen.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kCurvatureStepMeters = 5.0f;
constexpr float kGravity = 9.8f;

float computeCurvatureRadiusXZ(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2) {
    glm::vec2 a(p0.x, p0.z);
    glm::vec2 b(p1.x, p1.z);
    glm::vec2 c(p2.x, p2.z);
    const float ab = glm::length(b - a);
    const float bc = glm::length(c - b);
    const float ca = glm::length(a - c);
    const float twiceArea = std::abs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x));
    if (ab < 1e-5f || bc < 1e-5f || ca < 1e-5f || twiceArea < 1e-5f)
        return 0.0f;
    return (ab * bc * ca) / (2.0f * twiceArea);
}

glm::vec3 sampleAtDistance(const std::vector<CurvePt>& curve,
                           const std::vector<float>& arc,
                           float distance) {
    if (curve.empty()) return {};
    if (curve.size() == 1 || arc.empty()) return curve.front().pos;
    if (distance <= 0.0f) return curve.front().pos;
    if (distance >= arc.back()) return curve.back().pos;

    auto it = std::lower_bound(arc.begin(), arc.end(), distance);
    size_t idx = static_cast<size_t>(std::max<std::ptrdiff_t>(1, it - arc.begin())) - 1;
    float span = std::max(arc[idx + 1] - arc[idx], 1e-6f);
    float t = std::clamp((distance - arc[idx]) / span, 0.0f, 1.0f);
    return glm::mix(curve[idx].pos, curve[idx + 1].pos, t);
}

float computeBankAngleRadians(float radius, float targetSpeedKmh, float friction) {
    if (radius <= 1e-5f)
        return 0.0f;
    const float speedMps = std::max(0.0f, targetSpeedKmh) * (1000.0f / 3600.0f);
    const float a = speedMps * speedMps / radius;
    float theta = std::atan2(a - kGravity * friction, kGravity + a * friction);
    if (theta < 0.0f)
        theta = 0.0f;
    return theta;
}

struct SamplePoint {
    float distance = 0.0f;
    float value = 0.0f;
};

float evaluateLinear(float distance, const std::vector<SamplePoint>& points, float fallback) {
    if (points.empty())
        return fallback;
    if (distance <= points.front().distance)
        return points.front().value;
    if (distance >= points.back().distance)
        return points.back().value;

    for (size_t i = 1; i < points.size(); ++i) {
        if (distance > points[i].distance)
            continue;
        float span = std::max(points[i].distance - points[i - 1].distance, 1e-6f);
        float t = std::clamp((distance - points[i - 1].distance) / span, 0.0f, 1.0f);
        return points[i - 1].value + (points[i].value - points[i - 1].value) * t;
    }
    return points.back().value;
}
}

std::vector<float> BankAngleGen::sampleAnglesRadians(
    const Road& road,
    const std::vector<CurvePt>& curve) {
    std::vector<float> result(curve.size(), 0.0f);
    if (curve.size() < 2)
        return result;

    std::vector<float> arc;
    arc.reserve(curve.size());
    arc.push_back(0.0f);
    for (size_t i = 1; i < curve.size(); ++i)
        arc.push_back(arc.back() + glm::length(curve[i].pos - curve[i - 1].pos));
    const float totalLength = arc.back();
    if (totalLength <= 1e-5f)
        return result;

    std::vector<SamplePoint> speedPoints;
    std::vector<SamplePoint> resolvedPoints;
    speedPoints.reserve(road.bankAngle.size());
    resolvedPoints.reserve(road.bankAngle.size());
    for (const auto& point : road.bankAngle) {
        const float pointDistance = std::clamp(point.u, 0.0f, 1.0f) * totalLength;
        speedPoints.push_back({pointDistance, std::max(0.0f, point.targetSpeed)});
        resolvedPoints.push_back({
            pointDistance,
            point.useAngle ? glm::radians(std::clamp(point.angle, -90.0f, 90.0f)) : 0.0f
        });
    }
    std::sort(speedPoints.begin(), speedPoints.end(),
              [](const SamplePoint& a, const SamplePoint& b) { return a.distance < b.distance; });
    std::sort(resolvedPoints.begin(), resolvedPoints.end(),
              [](const SamplePoint& a, const SamplePoint& b) { return a.distance < b.distance; });

    for (size_t i = 0; i < curve.size(); ++i) {
        const float distance = arc[i];
        const glm::vec3 p0 = sampleAtDistance(curve, arc, std::max(0.0f, distance - kCurvatureStepMeters));
        const glm::vec3 p1 = sampleAtDistance(curve, arc, distance);
        const glm::vec3 p2 = sampleAtDistance(curve, arc, std::min(totalLength, distance + kCurvatureStepMeters));
        const float radius = computeCurvatureRadiusXZ(p0, p1, p2);
        const float speed = evaluateLinear(distance, speedPoints, std::max(0.0f, road.defaultTargetSpeed));
        float value = computeBankAngleRadians(radius, speed, std::max(0.0f, road.defaultFriction));

        if (!road.bankAngle.empty()) {
            std::vector<SamplePoint> evalPoints;
            evalPoints.reserve(road.bankAngle.size());
            for (size_t j = 0; j < road.bankAngle.size(); ++j) {
                const auto& point = road.bankAngle[j];
                float pointDistance = std::clamp(point.u, 0.0f, 1.0f) * totalLength;
                const glm::vec3 q0 = sampleAtDistance(curve, arc, std::max(0.0f, pointDistance - kCurvatureStepMeters));
                const glm::vec3 q1 = sampleAtDistance(curve, arc, pointDistance);
                const glm::vec3 q2 = sampleAtDistance(curve, arc, std::min(totalLength, pointDistance + kCurvatureStepMeters));
                const float pointRadius = computeCurvatureRadiusXZ(q0, q1, q2);
                const float pointSpeed = evaluateLinear(pointDistance, speedPoints, std::max(0.0f, road.defaultTargetSpeed));
                const float autoAngle = computeBankAngleRadians(pointRadius, pointSpeed, std::max(0.0f, road.defaultFriction));
                evalPoints.push_back({
                    pointDistance,
                    point.useAngle ? glm::radians(std::clamp(point.angle, -90.0f, 90.0f)) : autoAngle
                });
            }
            std::sort(evalPoints.begin(), evalPoints.end(),
                      [](const SamplePoint& a, const SamplePoint& b) { return a.distance < b.distance; });
            value = evaluateLinear(distance, evalPoints, value);
        }

        result[i] = value;
    }

    return result;
}
