#pragma once

#include "../model/Road.h"
#include "../renderer/Mesh.h"
#include <vector>

// Generates a road surface quad-strip mesh from a Road's control points.
// Works in GL rendering space (X already flipped from world).
class RoadMeshGen {
public:
    // Appends generated vertices/indices into the output vectors (supports merging roads).
    static void generate(const Road&                   road,
                         std::vector<Mesh::Vertex>&   outVerts,
                         std::vector<uint32_t>&       outIndices,
                         int                          subdiv = 6);

private:
    // Left-handed world (X+ left) → right-handed GL (X+ right)
    static glm::vec3 toGL(const glm::vec3& p) { return {-p.x, p.y, p.z}; }
};
