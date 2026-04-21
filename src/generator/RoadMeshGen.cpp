#include "RoadMeshGen.h"
#include "ClothoidGen.h"
#include "BankAngleGen.h"
#include "LaneSectionGen.h"
#include "VerticalCurveGen.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {

std::vector<float> buildSideOffsets(float width, float divideLength) {
    std::vector<float> offsets;
    const float clampedWidth = std::max(width, 0.0f);
    const float step = std::max(divideLength, 0.01f);
    const int fullSegments = static_cast<int>(std::floor(clampedWidth / step));
    const float remainder = clampedWidth - static_cast<float>(fullSegments) * step;
    constexpr float kEps = 1e-4f;

    if (remainder > kEps)
        offsets.push_back(clampedWidth);

    for (int seg = fullSegments; seg >= 1; --seg)
        offsets.push_back(static_cast<float>(seg) * step);

    return offsets;
}

uint32_t sideVertexIndex(uint32_t rowStart, int centerIndex, int stepFromCenter, int dir) {
    return rowStart + static_cast<uint32_t>(centerIndex + dir * stepFromCenter);
}

}

void RoadMeshGen::generate(const Road&                 road,
                            std::vector<Mesh::Vertex>& outVerts,
                            std::vector<uint32_t>&     outIndices,
                            int                        /*subdiv*/,
                            const RoadNetwork*         /*net*/)
{
    const auto& pts = road.points;
    if (pts.size() < 2 || road.active == 0) return;

    // --- 1. Build smooth centerline using clothoid curves ---
    const float sampleInterval = std::max(road.segmentLength, 0.01f);
    auto baseCurve = ClothoidGen::buildCenterlineDetailedResampled(
        pts, sampleInterval, road.equalMidpoint);
    auto curve = VerticalCurveGen::applyDetailed(road, baseCurve, sampleInterval);
    auto bankAngles = BankAngleGen::sampleAnglesRadians(road, curve);
    auto frames = BankAngleGen::sampleFrames(curve, bankAngles);

    std::vector<glm::vec3> worldSamples;
    worldSamples.reserve(curve.size());
    for (const auto& pt : curve)
        worldSamples.push_back(pt.pos);

    if (worldSamples.size() < 2) return;
    const int N = static_cast<int>(worldSamples.size());

    // --- 3. Vertices: grid per sample, with ~1 m spacing across the road width ---
    const uint32_t base = static_cast<uint32_t>(outVerts.size());
    float cumDist = 0.0f;
    const float totalDist = [&]() {
        float v = 0.0f;
        for (int i = 1; i < N; ++i)
            v += glm::length(worldSamples[i] - worldSamples[i - 1]);
        return std::max(v, 1e-6f);
    }();

    std::vector<std::vector<float>> leftOffsetsPerRow;
    std::vector<std::vector<float>> rightOffsetsPerRow;
    std::vector<int> leftSegmentCounts;
    std::vector<int> rightSegmentCounts;
    leftOffsetsPerRow.reserve(N);
    rightOffsetsPerRow.reserve(N);
    leftSegmentCounts.reserve(N);
    rightSegmentCounts.reserve(N);
    const float divideLength = std::max(road.divideLength, 0.01f);
    for (int i = 0; i < N; ++i) {
        if (i > 0) cumDist += glm::length(worldSamples[i] - worldSamples[i-1]);

        const CurveFrame& frame = frames[std::min<int>(i, (int)frames.size() - 1)];
        glm::vec3 right = frame.right;
        glm::vec3 up = frame.up;

        const float u = cumDist / totalDist;
        const EvaluatedLaneSection lane = LaneSectionGen::evaluateAtU(road, u);
        float leftW = lane.widthLeft2 + lane.widthLeft1 + lane.widthCenter * 0.5f;
        float rightW = lane.widthRight1 + lane.widthRight2 + lane.widthCenter * 0.5f;
        if (leftW < 1e-4f && rightW < 1e-4f) leftW = rightW = 0.1f;
        glm::vec3 shiftedCenter = worldSamples[i] + right * lane.offsetCenter;

        const std::vector<float> leftOffsets = buildSideOffsets(leftW, divideLength);
        const std::vector<float> rightOffsets = buildSideOffsets(rightW, divideLength);
        leftOffsetsPerRow.emplace_back(leftOffsets.rbegin(), leftOffsets.rend());
        rightOffsetsPerRow.emplace_back(rightOffsets.rbegin(), rightOffsets.rend());
        leftSegmentCounts.push_back(static_cast<int>(leftOffsets.size()));
        rightSegmentCounts.push_back(static_cast<int>(rightOffsets.size()));

        for (float offset : leftOffsets) {
            const float t = (leftW > 1e-4f) ? (offset / leftW) : 0.0f;
            const glm::vec3 pos = shiftedCenter + right * offset;
            outVerts.push_back({ toGL(pos), toGL(up), {0.5f - 0.5f * t, cumDist} });
        }

        outVerts.push_back({ toGL(shiftedCenter), toGL(up), {0.5f, cumDist} });

        for (auto it = rightOffsets.rbegin(); it != rightOffsets.rend(); ++it) {
            const float offset = *it;
            const float t = (rightW > 1e-4f) ? (offset / rightW) : 0.0f;
            const glm::vec3 pos = shiftedCenter - right * offset;
            outVerts.push_back({ toGL(pos), toGL(up), {0.5f + 0.5f * t, cumDist} });
        }
    }

    // --- 4. Indices: triangulate left and right sides separately around the centerline ---
    uint32_t rowStart = base;
    for (int i = 0; i + 1 < N; ++i) {
        const int left0 = leftSegmentCounts[i];
        const int right0 = rightSegmentCounts[i];
        const int left1 = leftSegmentCounts[i + 1];
        const int right1 = rightSegmentCounts[i + 1];
        const int cols0 = left0 + 1 + right0;
        const int cols1 = left1 + 1 + right1;
        const uint32_t nextRowStart = rowStart + static_cast<uint32_t>(cols0);

        const auto emitBand = [&](const std::vector<float>& offsets0,
                                  const std::vector<float>& offsets1,
                                  int centerIndex0, int centerIndex1, int dir) {
            int i0 = 0;
            int i1 = 0;

            while (i0 < static_cast<int>(offsets0.size()) || i1 < static_cast<int>(offsets1.size())) {
                const bool canAdvance0 = i0 < static_cast<int>(offsets0.size());
                const bool canAdvance1 = i1 < static_cast<int>(offsets1.size());
                const float next0 = canAdvance0 ? offsets0[i0] : std::numeric_limits<float>::infinity();
                const float next1 = canAdvance1 ? offsets1[i1] : std::numeric_limits<float>::infinity();

                if (next0 <= next1) {
                    const uint32_t v00 = sideVertexIndex(rowStart, centerIndex0, i0, dir);
                    const uint32_t v01 = sideVertexIndex(rowStart, centerIndex0, i0 + 1, dir);
                    const uint32_t v10 = sideVertexIndex(nextRowStart, centerIndex1, i1, dir);
                    outIndices.push_back(v00); outIndices.push_back(v10); outIndices.push_back(v01);
                    ++i0;
                } else {
                    const uint32_t v00 = sideVertexIndex(rowStart, centerIndex0, i0, dir);
                    const uint32_t v10 = sideVertexIndex(nextRowStart, centerIndex1, i1, dir);
                    const uint32_t v11 = sideVertexIndex(nextRowStart, centerIndex1, i1 + 1, dir);
                    outIndices.push_back(v00); outIndices.push_back(v10); outIndices.push_back(v11);
                    ++i1;
                }
            }
        };

        emitBand(leftOffsetsPerRow[i], leftOffsetsPerRow[i + 1], left0, left1, -1);
        emitBand(rightOffsetsPerRow[i], rightOffsetsPerRow[i + 1], left0, left1, +1);

        rowStart = nextRowStart;
    }
}
