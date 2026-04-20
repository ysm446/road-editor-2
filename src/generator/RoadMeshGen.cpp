#include "RoadMeshGen.h"
#include "ClothoidGen.h"
#include "BankAngleGen.h"
#include "LaneSectionGen.h"
#include "VerticalCurveGen.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>

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

    // --- 3. Vertices: two per sample (left edge, right edge) ---
    const uint32_t base = static_cast<uint32_t>(outVerts.size());
    float cumDist = 0.0f;
    const float totalDist = [&]() {
        float v = 0.0f;
        for (int i = 1; i < N; ++i)
            v += glm::length(worldSamples[i] - worldSamples[i - 1]);
        return std::max(v, 1e-6f);
    }();

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

        float vCoord = cumDist / (leftW + rightW);

        outVerts.push_back({ toGL(shiftedCenter + right * leftW),  toGL(up), {0.0f, vCoord} });
        outVerts.push_back({ toGL(shiftedCenter - right * rightW), toGL(up), {1.0f, vCoord} });
    }

    // --- 4. Indices: two CCW triangles per quad ---
    for (int i = 0; i + 1 < N; ++i) {
        uint32_t L0 = base + uint32_t(i * 2);
        uint32_t R0 = base + uint32_t(i * 2 + 1);
        uint32_t L1 = base + uint32_t(i * 2 + 2);
        uint32_t R1 = base + uint32_t(i * 2 + 3);

        outIndices.push_back(L0); outIndices.push_back(L1); outIndices.push_back(R0);
        outIndices.push_back(R0); outIndices.push_back(L1); outIndices.push_back(R1);
    }
}
