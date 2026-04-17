#include "RoadRenderer.h"
#include "../generator/RoadMeshGen.h"
#include <glm/gtc/matrix_transform.hpp>

void RoadRenderer::init(QOpenGLFunctions_4_1_Core* f) {
    m_roads.init(f);
    m_nodes.init(f);
    m_selBatch.init(f);
    m_surfaceMesh.init(f);
    m_ready = true;
}

void RoadRenderer::rebuild(QOpenGLFunctions_4_1_Core* f, const RoadNetwork& net) {
    if (!m_ready) return;

    const glm::vec3 kRoad  = {0.95f, 0.85f, 0.35f};
    const glm::vec3 kNode  = {0.35f, 0.90f, 0.45f};
    const float     kCross = 5.0f;

    // Centerlines
    m_roads.begin();
    for (const auto& road : net.roads) {
        if (road.points.size() < 2) continue;
        for (size_t i = 0; i + 1 < road.points.size(); ++i)
            m_roads.addLine(toGL(road.points[i    ].pos),
                            toGL(road.points[i + 1].pos), kRoad);
    }
    m_roads.upload(f);

    // Intersection markers
    m_nodes.begin();
    for (const auto& n : net.intersections) {
        glm::vec3 p = toGL(n.pos);
        m_nodes.addLine(p - glm::vec3(kCross, 0, 0), p + glm::vec3(kCross, 0, 0), kNode);
        m_nodes.addLine(p - glm::vec3(0, kCross, 0), p + glm::vec3(0, kCross, 0), kNode);
        m_nodes.addLine(p - glm::vec3(0, 0, kCross), p + glm::vec3(0, 0, kCross), kNode);
    }
    m_nodes.upload(f);

    // Road surface mesh (all roads merged)
    std::vector<Mesh::Vertex> verts;
    std::vector<uint32_t>     indices;
    for (const auto& road : net.roads)
        RoadMeshGen::generate(road, verts, indices);
    m_surfaceMesh.upload(f, verts, indices);

    m_hasData = true;
}

void RoadRenderer::updateSelection(QOpenGLFunctions_4_1_Core* f,
                                   const RoadNetwork& net, int roadIdx, int pointIdx)
{
    if (!m_ready) return;
    const glm::vec3 kSel   = {1.0f, 0.55f, 0.0f}; // orange
    const float     kCross = 6.0f;

    m_selBatch.begin();
    if (roadIdx >= 0 && pointIdx >= 0 &&
        roadIdx < (int)net.roads.size() &&
        pointIdx < (int)net.roads[roadIdx].points.size())
    {
        glm::vec3 p = toGL(net.roads[roadIdx].points[pointIdx].pos);
        m_selBatch.addLine(p - glm::vec3(kCross, 0, 0), p + glm::vec3(kCross, 0, 0), kSel);
        m_selBatch.addLine(p - glm::vec3(0, kCross, 0), p + glm::vec3(0, kCross, 0), kSel);
        m_selBatch.addLine(p - glm::vec3(0, 0, kCross), p + glm::vec3(0, 0, kCross), kSel);
    }
    m_selBatch.upload(f);
}

void RoadRenderer::draw(QOpenGLFunctions_4_1_Core* f,
                        Shader& lineShader, Shader& roadShader,
                        const glm::mat4& vp)
{
    if (!m_ready || !m_hasData) return;

    // --- Road surface (draw first so lines appear on top) ---
    f->glDisable(GL_CULL_FACE);
    f->glEnable(GL_POLYGON_OFFSET_FILL);
    f->glPolygonOffset(2.0f, 2.0f); // push surface behind lines

    roadShader.bind();
    roadShader.setMat4(f, "u_mvp",    vp);
    roadShader.setVec3(f, "u_sunDir", glm::normalize(glm::vec3(0.4f, 1.0f, 0.5f)));
    roadShader.setVec3(f, "u_color",  glm::vec3(0.30f, 0.30f, 0.30f));
    m_surfaceMesh.draw(f);
    roadShader.unbind();

    f->glDisable(GL_POLYGON_OFFSET_FILL);

    // --- Centerlines, markers and selection on top ---
    m_roads.draw(f, lineShader, vp);
    m_nodes.draw(f, lineShader, vp);
    m_selBatch.draw(f, lineShader, vp);
}

void RoadRenderer::destroy(QOpenGLFunctions_4_1_Core* f) {
    m_roads.destroy(f);
    m_nodes.destroy(f);
    m_selBatch.destroy(f);
    m_surfaceMesh.destroy(f);
    m_ready   = false;
    m_hasData = false;
}
