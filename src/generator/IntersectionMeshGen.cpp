#include "IntersectionMeshGen.h"
#include "ClothoidGen.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

// One road's entry cross-section at the intersection throat.
struct EntryEdge {
    glm::vec3 left;   // GL space left edge point
    glm::vec3 right;  // GL space right edge point
    float     angle;  // atan2 angle of the cut-point around intersection center (for sort)
};

// Walk centerline from 'fromEnd' (0=start, 1=end) until arc-length >= dist.
// Returns the interpolated cut point and the tangent direction at that point.
// All coordinates are in world space.
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
            if (fromEnd) outTangent = -outTangent; // point away from intersection
            return true;
        }
        accumulated += segLen;
    }
    // dist > total length — use endpoint
    outPos = fromEnd ? cl.front() : cl.back();
    int li = fromEnd ? 1 : (n - 2);
    int hi = fromEnd ? 0 : (n - 1);
    glm::vec3 dir = cl[hi] - cl[li];
    float len = glm::length(dir);
    outTangent = (len > 1e-6f) ? dir / len : glm::vec3(0, 0, 1);
    if (fromEnd) outTangent = -outTangent;
    return true;
}

// Compute total left / right profile widths from active lane flags.
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
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 normal(0.0f, 1.0f, 0.0f);

    // --- 1. Collect entry edges from all roads connected to this intersection ---
    std::vector<EntryEdge> edges;

    for (const auto& road : net.roads) {
        bool atStart = road.startIntersectionId == ix.id;
        bool atEnd   = road.endIntersectionId   == ix.id;
        if (!atStart && !atEnd) continue;
        if (road.points.size() < 2 || road.active == 0) continue;

        float entryDist = ix.entryDist;
        if (entryDist < 0.1f) entryDist = 0.1f;

        // Build a fine centerline (world space)
        auto cl = ClothoidGen::buildCenterline(road.points, 0.5f, road.equalMidpoint);
        if (cl.size() < 2) continue;

        glm::vec3 cutPos, tangent;
        bool fromEnd = atEnd; // if road ends here, walk from back
        if (!findCutPoint(cl, fromEnd, entryDist, cutPos, tangent)) continue;

        float leftW, rightW;
        laneWidths(road, leftW, rightW);

        // Binormal: cross(worldUp, tangent), pointing right of travel direction
        glm::vec3 binom = glm::cross(worldUp, tangent);
        float bLen = glm::length(binom);
        if (bLen < 1e-6f) continue;
        binom /= bLen;

        // Left/right edge in world space → convert to GL space
        glm::vec3 leftWorld  = cutPos - binom * leftW;
        glm::vec3 rightWorld = cutPos + binom * rightW;

        // Angle of cutPos around intersection center (XZ plane, world coords)
        float angle = std::atan2(cutPos.z - ix.pos.z, cutPos.x - ix.pos.x);

        EntryEdge e;
        e.left  = toGL(leftWorld);
        e.right = toGL(rightWorld);
        e.angle = angle;
        edges.push_back(e);
    }

    if (edges.size() < 2) return; // need at least two roads

    // --- 2. Sort by angle around intersection center ---
    std::sort(edges.begin(), edges.end(),
              [](const EntryEdge& a, const EntryEdge& b) { return a.angle < b.angle; });

    // --- 3. Fan-triangulate from center ---
    // For each adjacent pair of entry edges, determine which side faces the center
    // and emit a triangle: (center, innerEdgeA, innerEdgeB)

    glm::vec3 center = toGL(ix.pos);
    // Raise slightly above road surface to avoid z-fighting with road mesh
    center.y += 0.002f;

    const uint32_t base = static_cast<uint32_t>(outVerts.size());
    outVerts.push_back({ center, normal, {0.5f, 0.5f} });

    // Add all edge points (left and right of each entry)
    // Determine which edge (left or right) is the "inner" edge
    // (closer to intersection center) for each road entry.
    struct EdgePair {
        glm::vec3 inner, outer;
    };
    std::vector<EdgePair> pairs;
    pairs.reserve(edges.size());

    for (const auto& e : edges) {
        float distL = glm::distance(e.left,  center);
        float distR = glm::distance(e.right, center);
        if (distL < distR)
            pairs.push_back({ e.left,  e.right });
        else
            pairs.push_back({ e.right, e.left  });
    }

    // Add inner edge vertices
    uint32_t innerBase = base + 1;
    for (const auto& p : pairs)
        outVerts.push_back({ p.inner, normal, {0.0f, 0.0f} });

    // Emit triangles: (center, innerA, innerB) for each consecutive pair (wrapping)
    int N = (int)pairs.size();
    for (int i = 0; i < N; ++i) {
        int j = (i + 1) % N;
        // Fan triangle from center to adjacent inner edges
        outIndices.push_back(base);                      // center
        outIndices.push_back(innerBase + (uint32_t)i);   // innerA
        outIndices.push_back(innerBase + (uint32_t)j);   // innerB
    }
}
