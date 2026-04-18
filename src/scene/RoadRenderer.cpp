#include "RoadRenderer.h"
#include "../generator/RoadMeshGen.h"
#include <glm/gtc/matrix_transform.hpp>

void RoadRenderer::init(QOpenGLFunctions_4_1_Core* f) {
    m_roads.init(f);
    m_nodes.init(f);
    m_selBatch.init(f);
    m_selRoad.init(f);
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
    const glm::vec3 kSelPt   = {1.0f, 0.55f, 0.0f};  // orange — control point cross
    const glm::vec3 kSelRoad = {0.2f, 0.9f, 1.0f};    // cyan — selected road
    const float     kCross   = 6.0f;

    // Selected control point (single point for GL_POINTS rendering)
    m_selBatch.begin();
    if (roadIdx >= 0 && pointIdx >= 0 &&
        roadIdx < (int)net.roads.size() &&
        pointIdx < (int)net.roads[roadIdx].points.size())
    {
        glm::vec3 p = toGL(net.roads[roadIdx].points[pointIdx].pos);
        m_selBatch.addPoint(p, kSelPt);
    }
    m_selBatch.upload(f);

    // Selected road centerline highlight
    m_selRoad.begin();
    if (roadIdx >= 0 && roadIdx < (int)net.roads.size()) {
        const auto& road = net.roads[roadIdx];
        for (size_t i = 0; i + 1 < road.points.size(); ++i)
            m_selRoad.addLine(toGL(road.points[i    ].pos),
                              toGL(road.points[i + 1].pos), kSelRoad);
    }
    m_selRoad.upload(f);
}

void RoadRenderer::draw(QOpenGLFunctions_4_1_Core* f,
                        Shader& lineShader, Shader& roadShader, Shader& pointShader,
                        const glm::mat4& vp)
{
    if (!m_ready || !m_hasData) return;

    // --- Road surface (draw first so lines appear on top) ---
    f->glDisable(GL_CULL_FACE);

    // Solid surface (always drawn)
    f->glEnable(GL_POLYGON_OFFSET_FILL);
    f->glPolygonOffset(2.0f, 2.0f);

    roadShader.bind();
    roadShader.setMat4(f, "u_mvp",    vp);
    roadShader.setVec3(f, "u_sunDir", glm::normalize(glm::vec3(0.4f, 1.0f, 0.5f)));
    roadShader.setVec3(f, "u_color",  glm::vec3(0.30f, 0.30f, 0.30f));
    m_surfaceMesh.draw(f);
    roadShader.unbind();

    f->glDisable(GL_POLYGON_OFFSET_FILL);

    // Wireframe overlay (only when enabled)
    if (m_wireframe) {
        f->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        f->glEnable(GL_POLYGON_OFFSET_LINE);
        f->glPolygonOffset(-1.0f, -1.0f);

        roadShader.bind();
        roadShader.setMat4(f, "u_mvp",    vp);
        roadShader.setVec3(f, "u_sunDir", glm::normalize(glm::vec3(0.4f, 1.0f, 0.5f)));
        roadShader.setVec3(f, "u_color",  glm::vec3(0.30f, 0.65f, 0.30f));
        m_surfaceMesh.draw(f);
        roadShader.unbind();

        f->glDisable(GL_POLYGON_OFFSET_LINE);
        f->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // --- Centerlines, markers and selection on top ---
    m_roads.draw(f, lineShader, vp);
    m_nodes.draw(f, lineShader, vp);

    // Selected road: pull forward so it wins the depth test over m_roads
    f->glEnable(GL_POLYGON_OFFSET_LINE);
    f->glPolygonOffset(-1.0f, -1.0f);
    m_selRoad.draw(f, lineShader, vp);
    f->glDisable(GL_POLYGON_OFFSET_LINE);

    // Control point dot (screen-space circle)
    f->glEnable(GL_PROGRAM_POINT_SIZE);
    m_selBatch.drawPoints(f, pointShader, vp, 12.0f);
    f->glDisable(GL_PROGRAM_POINT_SIZE);
}

void RoadRenderer::destroy(QOpenGLFunctions_4_1_Core* f) {
    m_roads.destroy(f);
    m_nodes.destroy(f);
    m_selBatch.destroy(f);
    m_selRoad.destroy(f);
    m_surfaceMesh.destroy(f);
    m_ready   = false;
    m_hasData = false;
}
