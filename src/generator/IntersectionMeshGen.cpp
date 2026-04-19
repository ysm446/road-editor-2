#include "IntersectionMeshGen.h"
#include "ClothoidGen.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

// Walk centerline from 'fromEnd' (0=start, 1=end) until arc-length >= dist.
static bool findCutPoint(const std::vector<glm::vec3>& cl, bool fromEnd,
                         float dist, glm::vec3& outPos, glm::vec3& outTangent)
{
    if (cl.size() < 2) return false;

    float accumulated = 0.0f;
    auto getIdx = [&](int i) -> const glm::vec3& {
        return fromEnd ? cl[cl.size() - 1 - i] : cl[i];
    };
    int n = (int)cl.size();
    for (int i = 0; i + 1 < n; ++i) {
        const glm::vec3& a = getIdx(i);
        const glm::vec3& b = getIdx(i + 1);
        float segLen = glm::distance(a, b);
        if (segLen < 1e-6f) continue;
        if (accumulated + segLen >= dist) {
            float t = (dist - accumulated) / segLen;
            outPos     = glm::mix(a, b, t);
            glm::vec3 dir = b - a;
            float len = glm::length(dir);
            outTangent = (len > 1e-6f) ? dir / len : glm::vec3(0, 0, 1);
            if (fromEnd) outTangent = -outTangent;
            return true;
        }
        accumulated += segLen;
    }
    outPos = fromEnd ? cl.front() : cl.back();
    int li = fromEnd ? 1 : (n - 2);
    int hi = fromEnd ? 0 : (n - 1);
    glm::vec3 dir = cl[hi] - cl[li];
    float len = glm::length(dir);
    outTangent = (len > 1e-6f) ? dir / len : glm::vec3(0, 0, 1);
    if (fromEnd) outTangent = -outTangent;
    return true;
}

static void laneWidths(const Road& road, float& leftW, float& rightW) {
    leftW = rightW = 0.0f;
    if (road.useLaneLeft2)  leftW  += road.defaultWidthLaneLeft2;
    if (road.useLaneLeft1)  leftW  += road.defaultWidthLaneLeft1;
    if (road.useLaneCenter) { leftW += road.defaultWidthLaneCenter * 0.5f;
                              rightW += road.defaultWidthLaneCenter * 0.5f; }
    if (road.useLaneRight1) rightW += road.defaultWidthLaneRight1;
    if (road.useLaneRight2) rightW += road.defaultWidthLaneRight2;
    if (leftW < 1e-4f && rightW < 1e-4f) leftW = rightW = 0.1f;
}

// Quadratic Bezier: P0 → ctrl → P1 at parameter t.
static glm::vec3 bezier2(const glm::vec3& p0, const glm::vec3& ctrl,
                         const glm::vec3& p1, float t)
{
    float u = 1.0f - t;
    return u*u*p0 + 2.0f*u*t*ctrl + t*t*p1;
}

// Line intersection in the XZ plane.
// Finds K such that p1 + s*d1 == p2 + t*d2 (for some s, t).
// Returns false if lines are parallel.
static bool lineIntersectXZ(const glm::vec3& p1, const glm::vec3& d1,
                             const glm::vec3& p2, const glm::vec3& d2,
                             glm::vec3& outK)
{
    // 2D cross product of d1 x d2 in XZ
    float denom = d1.x * d2.z - d1.z * d2.x;
    if (std::abs(denom) < 1e-6f) return false;
    float dx = p2.x - p1.x;
    float dz = p2.z - p1.z;
    float s   = (dx * d2.z - dz * d2.x) / denom;
    outK   = p1 + s * d1;
    outK.y = (p1.y + p2.y) * 0.5f;
    return true;
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

    // --- 1. Collect entry edge points from all connected roads ---
    // Each road contributes left + right edge at entryDist, plus the road tangent
    // at that cut (pointing away from the intersection) for arc control-point math.

    struct BoundaryPt {
        glm::vec3 posWorld;      // world space (for arc geometry)
        glm::vec3 posGL;         // GL space (for output mesh)
        glm::vec3 tangentWorld;  // unit tangent pointing AWAY from intersection
        float     angle;         // atan2 around intersection center (for sort)
        int       roadIdx;       // which road, to detect road-to-road gaps
    };
    std::vector<BoundaryPt> boundary;
    int roadIdx = 0;

    for (const auto& road : net.roads) {
        bool atStart = road.startIntersectionId == ix.id;
        bool atEnd   = road.endIntersectionId   == ix.id;
        if (!atStart && !atEnd) continue;
        if (road.points.size() < 2 || road.active == 0) continue;

        float entryDist = ix.entryDist;
        if (entryDist < 0.1f) entryDist = 0.1f;

        auto cl = ClothoidGen::buildCenterline(road.points, 0.5f, road.equalMidpoint);
        if (cl.size() < 2) continue;

        bool fromEnd = atEnd;
        glm::vec3 cutPos, tangent;
        if (!findCutPoint(cl, fromEnd, entryDist, cutPos, tangent)) continue;

        float leftW, rightW;
        laneWidths(road, leftW, rightW);

        glm::vec3 binom = glm::cross(worldUp, tangent);
        float bLen = glm::length(binom);
        if (bLen < 1e-6f) continue;
        binom /= bLen;

        glm::vec3 leftWorld  = cutPos - binom * leftW;
        glm::vec3 rightWorld = cutPos + binom * rightW;

        auto angleOf = [&](const glm::vec3& p) {
            return std::atan2(p.z - ix.pos.z, p.x - ix.pos.x);
        };

        boundary.push_back({ leftWorld,  toGL(leftWorld),  tangent, angleOf(leftWorld),  roadIdx });
        boundary.push_back({ rightWorld, toGL(rightWorld), tangent, angleOf(rightWorld), roadIdx });
        ++roadIdx;
    }

    if ((int)boundary.size() < 4) return;

    // --- 2. Sort all boundary points by angle around intersection center ---
    std::sort(boundary.begin(), boundary.end(),
              [](const BoundaryPt& a, const BoundaryPt& b) { return a.angle < b.angle; });

    // --- 3. Build polygon with Bezier arcs in the gaps between different roads ---
    // For consecutive boundary points from the SAME road: straight line (road cross-section).
    // For consecutive boundary points from DIFFERENT roads: quadratic Bezier arc.
    //   Control point = intersection of the two tangent lines extended INTO the gap.
    //   This creates a smooth inward-curved corner between the roads.

    struct PolyPt { glm::vec3 posGL; };
    std::vector<PolyPt> poly;

    int M = (int)boundary.size();
    for (int i = 0; i < M; ++i) {
        const BoundaryPt& cur  = boundary[i];
        const BoundaryPt& nxt  = boundary[(i + 1) % M];

        poly.push_back({ cur.posGL });

        if (cur.roadIdx != nxt.roadIdx) {
            // Gap between two roads. Tangents point AWAY from intersection,
            // so -tangent points INTO the gap (toward the intersection interior).
            glm::vec3 K;
            if (lineIntersectXZ(cur.posWorld, -cur.tangentWorld,
                                 nxt.posWorld, -nxt.tangentWorld, K)) {
                // Sample arc interior points (skip endpoints: they are added as boundary pts)
                for (int k = 1; k < kArcSegs; ++k) {
                    float t    = float(k) / float(kArcSegs);
                    glm::vec3 p = bezier2(cur.posWorld, K, nxt.posWorld, t);
                    poly.push_back({ toGL(p) });
                }
            }
            // parallel roads: no arc, straight edge is fine
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
