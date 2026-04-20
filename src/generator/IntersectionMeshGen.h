#pragma once

#include "../model/RoadNetwork.h"
#include "../renderer/Mesh.h"
#include <vector>

class IntersectionMeshGen {
public:
    // Appends intersection fill geometry for one intersection into the output vectors.
    // Output is in rendering space (same as world space in the right-handed system).
    static void generate(const Intersection&          intersection,
                         const RoadNetwork&            net,
                         std::vector<Mesh::Vertex>&   outVerts,
                         std::vector<uint32_t>&       outIndices);

private:
    static glm::vec3 toGL(const glm::vec3& p) { return p; }
};
