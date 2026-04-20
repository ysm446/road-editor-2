#pragma once

#include "../model/Road.h"
#include "../model/RoadNetwork.h"
#include "../renderer/Mesh.h"
#include <vector>

// Generates a road surface quad-strip mesh from a Road's control points.
// Works directly in the app's right-handed world/rendering space.
class RoadMeshGen {
public:
    // Appends generated vertices/indices into the output vectors (supports merging roads).
    // Pass net != nullptr to enable intersection trimming (road endpoints cut by entryDist).
    static void generate(const Road&                   road,
                         std::vector<Mesh::Vertex>&   outVerts,
                         std::vector<uint32_t>&       outIndices,
                         int                          subdiv = 6,
                         const RoadNetwork*           net    = nullptr);

private:
    static glm::vec3 toGL(const glm::vec3& p) { return p; }
};
