#include "RoadMeshGen.h"
#include "ClothoidGen.h"
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
    std::vector<glm::vec3> worldSamples =
        ClothoidGen::buildCenterline(pts, sampleInterval, road.equalMidpoint);
    worldSamples = VerticalCurveGen::apply(road, worldSamples, sampleInterval);

    if (worldSamples.size() < 2) return;

    // Convert to GL space (flip X: world left-handed → GL right-handed)
    std::vector<glm::vec3> samples;
    samples.reserve(worldSamples.size());
    for (const auto& p : worldSamples)
        samples.push_back(toGL(p));

    const int N = static_cast<int>(samples.size());

    // --- 2. Tangents (central difference, clamped at ends) ---
    std::vector<glm::vec3> tangents(N);
    for (int i = 0; i < N; ++i) {
        glm::vec3 t;
        if      (i == 0)   t = samples[1] - samples[0];
        else if (i == N-1) t = samples[N-1] - samples[N-2];
        else               t = samples[i+1] - samples[i-1];
        float len = glm::length(t);
        tangents[i] = (len > 1e-6f) ? t / len : glm::vec3(0, 0, 1);
    }

    // --- 3. Profile widths from active lanes ---
    float leftW = 0.0f, rightW = 0.0f;
    if (road.useLaneLeft2)  leftW  += road.defaultWidthLaneLeft2;
    if (road.useLaneLeft1)  leftW  += road.defaultWidthLaneLeft1;
    if (road.useLaneCenter) { leftW += road.defaultWidthLaneCenter * 0.5f;
                              rightW += road.defaultWidthLaneCenter * 0.5f; }
    if (road.useLaneRight1) rightW += road.defaultWidthLaneRight1;
    if (road.useLaneRight2) rightW += road.defaultWidthLaneRight2;
    if (leftW < 1e-4f && rightW < 1e-4f) leftW = rightW = 0.1f;

    // --- 4. Vertices: two per sample (left edge, right edge) ---
    const uint32_t base = static_cast<uint32_t>(outVerts.size());
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    float cumDist = 0.0f;

    for (int i = 0; i < N; ++i) {
        if (i > 0) cumDist += glm::length(samples[i] - samples[i-1]);

        glm::vec3 t = tangents[i];
        glm::vec3 c = glm::cross(worldUp, t);
        glm::vec3 b = (glm::length(c) > 1e-6f) ? glm::normalize(c) : glm::vec3(1, 0, 0);
        glm::vec3 n = glm::normalize(glm::cross(t, b));

        float vCoord = cumDist / (leftW + rightW);

        outVerts.push_back({ samples[i] - b * leftW,  n, {0.0f, vCoord} });
        outVerts.push_back({ samples[i] + b * rightW, n, {1.0f, vCoord} });
    }

    // --- 5. Indices: two CCW triangles per quad ---
    for (int i = 0; i + 1 < N; ++i) {
        uint32_t L0 = base + uint32_t(i * 2);
        uint32_t R0 = base + uint32_t(i * 2 + 1);
        uint32_t L1 = base + uint32_t(i * 2 + 2);
        uint32_t R1 = base + uint32_t(i * 2 + 3);

        outIndices.push_back(L0); outIndices.push_back(L1); outIndices.push_back(R0);
        outIndices.push_back(R0); outIndices.push_back(L1); outIndices.push_back(R1);
    }
}
