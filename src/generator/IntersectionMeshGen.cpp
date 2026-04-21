#include "IntersectionMeshGen.h"
#include "ClothoidGen.h"
#include "LaneSectionGen.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

static void laneWidths(const Road& road, float u, float& leftW, float& rightW) {
    const EvaluatedLaneSection lane = LaneSectionGen::evaluateAtU(road, u);
    leftW = lane.widthLeft2 + lane.widthLeft1 + lane.widthCenter * 0.5f;
    rightW = lane.widthRight1 + lane.widthRight2 + lane.widthCenter * 0.5f;
    if (leftW < 1e-4f && rightW < 1e-4f) leftW = rightW = 0.1f;
}

static glm::vec3 bezier3(const glm::vec3& p0, const glm::vec3& c0,
                         const glm::vec3& c1, const glm::vec3& p1, float t)
{
    float u = 1.0f - t;
    return u*u*u*p0 + 3.0f*u*u*t*c0 + 3.0f*u*t*t*c1 + t*t*t*p1;
}

static void defaultLaneWidths(float& leftW, float& rightW) {
    const Road defaultRoad;
    laneWidths(defaultRoad, 0.0f, leftW, rightW);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
void IntersectionMeshGen::generate(
    const Intersection&          ix,
    const RoadNetwork&            net,
    std::vector<Mesh::Vertex>&   outVerts,
    std::vector<uint32_t>&       outIndices)
{
    static constexpr int   kArcSegs = 8;
    static constexpr float kYOffset = 0.003f;

    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);

    // --- 1. Collect boundary edge points from all connected road endpoints ---
    // Each road contributes left + right edge at the actual connected endpoint,
    // plus the road tangent at that endpoint (pointing away from the intersection)
    // for arc control-point math.

    struct BoundaryPt {
        glm::vec3 posWorld;      // world space (for arc geometry)
        glm::vec3 posGL;         // rendering space (same as world in RH mode)
        glm::vec3 tangentWorld;  // unit tangent pointing AWAY from intersection
        float     angle;         // atan2 around intersection center (for sort)
        int       roadIdx;       // which road, to detect road-to-road gaps
    };
    std::vector<BoundaryPt> boundary;
    int roadIdx = 0;

    for (const auto& road : net.roads) {
        bool atStart = road.startLink.connected() && road.startLink.intersectionId == ix.id;
        bool atEnd   = road.endLink.connected()   && road.endLink.intersectionId == ix.id;
        if (!atStart && !atEnd) continue;
        if (road.points.size() < 2 || road.active == 0) continue;

        auto cl = ClothoidGen::buildCenterline(road.points, 0.5f, road.equalMidpoint);
        if (cl.size() < 2) continue;

        glm::vec3 endpointPos;
        glm::vec3 tangent;
        if (atStart) {
            endpointPos = cl.front();
            glm::vec3 dir = cl[1] - cl[0];
            float len = glm::length(dir);
            if (len < 1e-6f) continue;
            tangent = dir / len;
        } else {
            endpointPos = cl.back();
            glm::vec3 dir = cl[(int)cl.size() - 1] - cl[(int)cl.size() - 2];
            float len = glm::length(dir);
            if (len < 1e-6f) continue;
            tangent = dir / len;
        }

        float leftW, rightW;
        laneWidths(road, atStart ? 0.0f : 1.0f, leftW, rightW);

        glm::vec3 binom = glm::cross(worldUp, tangent);
        float bLen = glm::length(binom);
        if (bLen < 1e-6f) continue;
        binom /= bLen;

        glm::vec3 leftWorld  = endpointPos + binom * leftW;
        glm::vec3 rightWorld = endpointPos - binom * rightW;

        auto angleOf = [&](const glm::vec3& p) {
            return std::atan2(p.z - ix.pos.z, p.x - ix.pos.x);
        };

        boundary.push_back({ leftWorld,  toGL(leftWorld),  tangent, angleOf(leftWorld),  roadIdx });
        boundary.push_back({ rightWorld, toGL(rightWorld), tangent, angleOf(rightWorld), roadIdx });
        ++roadIdx;
    }

    if ((int)boundary.size() < 4) {
        float leftW = 0.0f;
        float rightW = 0.0f;
        defaultLaneWidths(leftW, rightW);

        for (const auto& socket : ix.sockets) {
            if (!socket.enabled) continue;

            const glm::vec3 endpointPos = ix.pos + socket.localPos;
            const glm::vec3 tangent(std::cos(socket.yaw), 0.0f, std::sin(socket.yaw));
            const float tanLen = glm::length(tangent);
            if (tanLen < 1e-6f) continue;

            glm::vec3 binom = glm::cross(worldUp, tangent / tanLen);
            const float bLen = glm::length(binom);
            if (bLen < 1e-6f) continue;
            binom /= bLen;

            glm::vec3 leftWorld  = endpointPos + binom * leftW;
            glm::vec3 rightWorld = endpointPos - binom * rightW;

            auto angleOf = [&](const glm::vec3& p) {
                return std::atan2(p.z - ix.pos.z, p.x - ix.pos.x);
            };

            boundary.push_back({ leftWorld,  toGL(leftWorld),  tangent / tanLen, angleOf(leftWorld),  roadIdx });
            boundary.push_back({ rightWorld, toGL(rightWorld), tangent / tanLen, angleOf(rightWorld), roadIdx });
            ++roadIdx;
        }
    }

    if ((int)boundary.size() < 4) return;

    // --- 2. Sort all boundary points by angle around intersection center ---
    std::sort(boundary.begin(), boundary.end(),
              [](const BoundaryPt& a, const BoundaryPt& b) { return a.angle < b.angle; });

    // --- 3. Build polygon with Bezier arcs in the gaps between different roads ---
    // For consecutive boundary points from the SAME road: straight line (road cross-section).
    // For consecutive boundary points from DIFFERENT roads: cubic Bezier arc whose
    // endpoint tangents follow each road's endpoint direction.

    struct PolyPt { glm::vec3 posGL; };
    std::vector<PolyPt> poly;

    int M = (int)boundary.size();
    for (int i = 0; i < M; ++i) {
        const BoundaryPt& cur  = boundary[i];
        const BoundaryPt& nxt  = boundary[(i + 1) % M];

        poly.push_back({ cur.posGL });

        if (cur.roadIdx != nxt.roadIdx) {
            const float gapLen = glm::distance(cur.posWorld, nxt.posWorld);
            const float handleLen = std::max(0.5f, gapLen * 0.35f);
            glm::vec3 c0 = cur.posWorld + cur.tangentWorld * handleLen;
            glm::vec3 c1 = nxt.posWorld - nxt.tangentWorld * handleLen;
            c0.y = cur.posWorld.y;
            c1.y = nxt.posWorld.y;

            for (int k = 1; k < kArcSegs; ++k) {
                float t = float(k) / float(kArcSegs);
                glm::vec3 p = bezier3(cur.posWorld, c0, c1, nxt.posWorld, t);
                poly.push_back({ toGL(p) });
            }
        }
    }

    // --- 4. Fan-triangulate the polygon from the intersection center ---
    glm::vec3 center = toGL(ix.pos);
    center.y += kYOffset;

    const uint32_t base = static_cast<uint32_t>(outVerts.size());
    outVerts.push_back({ center, normal, {0.5f, 0.5f} });

    for (auto& pt : poly) {
        pt.posGL.y += kYOffset;
        outVerts.push_back({ pt.posGL, normal, {0.5f, 0.5f} });
    }

    int P = (int)poly.size();
    for (int i = 0; i < P; ++i) {
        int j = (i + 1) % P;
        outIndices.push_back(base);
        outIndices.push_back(base + 1 + (uint32_t)i);
        outIndices.push_back(base + 1 + (uint32_t)j);
    }
}
