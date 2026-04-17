#include "RoadMeshGen.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>

void RoadMeshGen::generate(const Road&                 road,
                            std::vector<Mesh::Vertex>& outVerts,
                            std::vector<uint32_t>&     outIndices,
                            int                        subdiv)
{
    const auto& pts = road.points;
    if (pts.size() < 2 || road.active == 0) return;

    // --- 1. Sample points along the polyline (GL space) ---
    std::vector<glm::vec3> samples;
    samples.reserve((pts.size() - 1) * subdiv + 1);

    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        glm::vec3 a = toGL(pts[i    ].pos);
        glm::vec3 b = toGL(pts[i + 1].pos);
        for (int k = 0; k < subdiv; ++k)
            samples.push_back(glm::mix(a, b, float(k) / float(subdiv)));
    }
    samples.push_back(toGL(pts.back().pos));

    const int N = static_cast<int>(samples.size());

    // --- 2. Tangents (central difference, clamped at ends) ---
    std::vector<glm::vec3> tangents(N);
    for (int i = 0; i < N; ++i) {
        glm::vec3 t;
        if (i == 0)     t = samples[1] - samples[0];
        else if (i == N-1) t = samples[N-1] - samples[N-2];
        else            t = samples[i+1] - samples[i-1];
        float len = glm::length(t);
        tangents[i] = (len > 1e-6f) ? t / len : glm::vec3(0, 0, 1);
    }

    // --- 3. Profile widths ---
    const float leftW  = road.defaultWidthLaneLeft1;
    const float rightW = road.defaultWidthLaneRight1;

    // --- 4. Vertices: two per sample (left edge, right edge) ---
    const uint32_t base = static_cast<uint32_t>(outVerts.size());
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    float cumDist = 0.0f;

    for (int i = 0; i < N; ++i) {
        if (i > 0) cumDist += glm::length(samples[i] - samples[i-1]);

        glm::vec3 t = tangents[i];
        glm::vec3 c = glm::cross(worldUp, t);
        glm::vec3 b = (glm::length(c) > 1e-6f) ? glm::normalize(c) : glm::vec3(1, 0, 0);
        glm::vec3 n = glm::normalize(glm::cross(t, b)); // surface normal

        float vCoord = cumDist / (leftW + rightW);

        // Left edge (u=0): center shifted toward -X in GL
        outVerts.push_back({ samples[i] - b * leftW,  n, {0.0f, vCoord} });
        // Right edge (u=1): center shifted toward +X in GL
        outVerts.push_back({ samples[i] + b * rightW, n, {1.0f, vCoord} });
    }

    // --- 5. Indices: two CCW triangles per quad ---
    // Winding for +Y normal (verified via cross product):
    //   tri1: L0, L1, R0   tri2: R0, L1, R1
    for (int i = 0; i + 1 < N; ++i) {
        uint32_t L0 = base + uint32_t(i * 2);
        uint32_t R0 = base + uint32_t(i * 2 + 1);
        uint32_t L1 = base + uint32_t(i * 2 + 2);
        uint32_t R1 = base + uint32_t(i * 2 + 3);

        outIndices.push_back(L0); outIndices.push_back(L1); outIndices.push_back(R0);
        outIndices.push_back(R0); outIndices.push_back(L1); outIndices.push_back(R1);
    }
}
