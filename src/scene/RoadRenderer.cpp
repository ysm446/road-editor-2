#include "RoadRenderer.h"
#include "../generator/RoadMeshGen.h"
#include "../generator/ClothoidGen.h"
#include "../generator/VerticalCurveGen.h"
#include "../generator/IntersectionMeshGen.h"
#include <set>
#include <glm/gtc/matrix_transform.hpp>

void RoadRenderer::init(QOpenGLFunctions_4_1_Core* f) {
    m_roads.init(f);
    m_nodes.init(f);
    m_socketLines.init(f);
    m_socketPoints.init(f);
    m_allPoints.init(f);
    m_allCtrlLines.init(f);
    m_selBatch.init(f);
    m_selRoad.init(f);
    m_selSockets.init(f);
    m_surfaceMesh.init(f);
    m_intersectionMesh.init(f);
    m_ready = true;
}

void RoadRenderer::rebuild(QOpenGLFunctions_4_1_Core* f, const RoadNetwork& net) {
    if (!m_ready) return;

    const glm::vec3 kStraight  = {1.0f, 1.0f, 1.0f};
    const glm::vec3 kClothoid  = {1.0f, 0.55f, 0.15f};
    const glm::vec3 kArc       = {0.2f, 0.9f, 0.2f};
    const glm::vec3 kVerticalCrest = {1.0f, 0.55f, 0.15f};
    const glm::vec3 kVerticalSag   = {0.45f, 0.85f, 1.0f};
    const glm::vec3 kNode      = {0.35f, 0.90f, 0.45f};
    const glm::vec3 kSocketLine = {0.55f, 0.65f, 0.90f};
    const glm::vec3 kSocketPoint = {0.70f, 0.80f, 1.00f};
    const float     kCross     = 5.0f;

    auto kindColor = [&](const CurvePt& p) -> const glm::vec3& {
        if (m_verticalCurvePreviewColors) {
            if (p.verticalKind == VerticalSegKind::Crest) return kVerticalCrest;
            if (p.verticalKind == VerticalSegKind::Sag)   return kVerticalSag;
            return kStraight;
        }
        SegKind k = p.kind;
        if (k == SegKind::Arc)      return kArc;
        if (k == SegKind::Clothoid) return kClothoid;
        return kStraight;
    };

    m_roads.begin();
    for (const auto& road : net.roads) {
        if (road.points.size() < 2) continue;
        auto baseCurve = ClothoidGen::buildCenterlineDetailed(
            road.points, road.segmentLength, road.equalMidpoint);
        auto curve = VerticalCurveGen::applyDetailed(road, baseCurve, road.segmentLength);
        for (size_t i = 0; i + 1 < curve.size(); ++i)
            m_roads.addLine(toGL(curve[i].pos), toGL(curve[i + 1].pos),
                            kindColor(curve[i]));
    }
    m_roads.upload(f);

    m_nodes.begin();
    for (const auto& n : net.intersections) {
        glm::vec3 p = toGL(n.pos);
        m_nodes.addLine(p - glm::vec3(kCross, 0, 0), p + glm::vec3(kCross, 0, 0), kNode);
        m_nodes.addLine(p - glm::vec3(0, kCross, 0), p + glm::vec3(0, kCross, 0), kNode);
        m_nodes.addLine(p - glm::vec3(0, 0, kCross), p + glm::vec3(0, 0, kCross), kNode);
    }
    m_nodes.upload(f);

    m_socketLines.begin();
    m_socketPoints.begin();
    for (const auto& n : net.intersections) {
        glm::vec3 center = toGL(n.pos);
        for (const auto& socket : n.sockets) {
            if (!socket.enabled) continue;
            glm::vec3 socketPos = toGL(n.pos + socket.localPos);
            m_socketLines.addLine(center, socketPos, kSocketLine);
            m_socketPoints.addPoint(socketPos, kSocketPoint);
        }
    }
    m_socketLines.upload(f);
    m_socketPoints.upload(f);

    const glm::vec3 kPtNormal = {0.75f, 0.75f, 0.75f};
    const glm::vec3 kCtrlLine = {0.45f, 0.45f, 0.45f};
    m_allPoints.begin();
    m_allCtrlLines.begin();
    for (const auto& road : net.roads) {
        for (const auto& cp : road.points)
            m_allPoints.addPoint(toGL(cp.pos), kPtNormal);
        for (size_t i = 0; i + 1 < road.points.size(); ++i)
            m_allCtrlLines.addLine(toGL(road.points[i].pos),
                                   toGL(road.points[i + 1].pos), kCtrlLine);
    }
    m_allPoints.upload(f);
    m_allCtrlLines.upload(f);

    std::vector<Mesh::Vertex> verts;
    std::vector<uint32_t> indices;
    for (const auto& road : net.roads)
        RoadMeshGen::generate(road, verts, indices, 6, &net);
    m_surfaceMesh.upload(f, verts, indices);

    std::vector<Mesh::Vertex> ixVerts;
    std::vector<uint32_t> ixIndices;
    for (const auto& ix : net.intersections)
        IntersectionMeshGen::generate(ix, net, ixVerts, ixIndices);
    m_intersectionMesh.upload(f, ixVerts, ixIndices);

    m_hasData = true;
}

void RoadRenderer::updateSelection(QOpenGLFunctions_4_1_Core* f,
                                   const RoadNetwork& net, const Selection& sel) {
    if (!m_ready) return;

    const glm::vec3 kSelPt   = {1.0f, 0.55f, 0.0f};
    const glm::vec3 kSelRoad = {0.2f, 0.9f, 1.0f};
    const glm::vec3 kSelSocket = {1.0f, 0.85f, 0.15f};

    m_selBatch.begin();
    for (const auto& pt : sel.points) {
        if (pt.roadIdx < 0 || pt.roadIdx >= (int)net.roads.size()) continue;
        if (pt.pointIdx < 0 || pt.pointIdx >= (int)net.roads[pt.roadIdx].points.size()) continue;
        m_selBatch.addPoint(toGL(net.roads[pt.roadIdx].points[pt.pointIdx].pos), kSelPt);
    }
    m_selBatch.upload(f);

    std::set<int> selectedRoads;
    if (sel.roadIdx >= 0) selectedRoads.insert(sel.roadIdx);
    for (const auto& pt : sel.points)
        if (pt.roadIdx >= 0)
            selectedRoads.insert(pt.roadIdx);

    m_selRoad.begin();
    for (int roadIdx : selectedRoads) {
        if (roadIdx < 0 || roadIdx >= (int)net.roads.size()) continue;
        const auto& road = net.roads[roadIdx];
        auto baseCurve = ClothoidGen::buildCenterlineDetailed(
            road.points, road.segmentLength, road.equalMidpoint);
        auto curve = VerticalCurveGen::applyDetailed(road, baseCurve, road.segmentLength);
        for (size_t i = 0; i + 1 < curve.size(); ++i)
            m_selRoad.addLine(toGL(curve[i].pos),
                              toGL(curve[i + 1].pos), kSelRoad);
    }
    m_selRoad.upload(f);

    m_selSockets.begin();
    if (sel.hasIntersection() &&
        sel.intersectionIdx >= 0 &&
        sel.intersectionIdx < (int)net.intersections.size()) {
        const auto& ix = net.intersections[sel.intersectionIdx];
        for (int i = 0; i < (int)ix.sockets.size(); ++i) {
            const auto& socket = ix.sockets[i];
            if (!socket.enabled) continue;
            if (sel.hasSocket() && sel.socketIdx != i) continue;
            m_selSockets.addPoint(toGL(ix.pos + socket.localPos), kSelSocket);
        }
    }
    m_selSockets.upload(f);
}

void RoadRenderer::draw(QOpenGLFunctions_4_1_Core* f,
                        Shader& lineShader, Shader& roadShader, Shader& pointShader,
                        const glm::mat4& vp) {
    if (!m_ready || !m_hasData) return;

    f->glDisable(GL_CULL_FACE);

    f->glEnable(GL_POLYGON_OFFSET_FILL);
    f->glPolygonOffset(2.0f, 2.0f);

    roadShader.bind();
    roadShader.setMat4(f, "u_mvp", vp);
    roadShader.setVec3(f, "u_sunDir", glm::normalize(glm::vec3(0.4f, 1.0f, 0.5f)));
    roadShader.setVec3(f, "u_color", glm::vec3(0.30f, 0.30f, 0.30f));
    m_surfaceMesh.draw(f);
    roadShader.setVec3(f, "u_color", glm::vec3(0.25f, 0.25f, 0.28f));
    m_intersectionMesh.draw(f);
    roadShader.unbind();

    f->glDisable(GL_POLYGON_OFFSET_FILL);

    if (m_wireframe) {
        f->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        f->glEnable(GL_POLYGON_OFFSET_LINE);
        f->glPolygonOffset(-1.0f, -1.0f);

        roadShader.bind();
        roadShader.setMat4(f, "u_mvp", vp);
        roadShader.setVec3(f, "u_sunDir", glm::normalize(glm::vec3(0.4f, 1.0f, 0.5f)));
        roadShader.setVec3(f, "u_color", glm::vec3(0.20f, 0.20f, 0.22f));
        m_surfaceMesh.draw(f);
        m_intersectionMesh.draw(f);
        roadShader.unbind();

        f->glDisable(GL_POLYGON_OFFSET_LINE);
        f->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (m_showPoints)
        m_allCtrlLines.draw(f, lineShader, vp);
    m_roads.draw(f, lineShader, vp);
    m_nodes.draw(f, lineShader, vp);
    m_socketLines.draw(f, lineShader, vp);

    f->glEnable(GL_POLYGON_OFFSET_LINE);
    f->glPolygonOffset(-1.0f, -1.0f);
    m_selRoad.draw(f, lineShader, vp);
    f->glDisable(GL_POLYGON_OFFSET_LINE);

    f->glEnable(GL_PROGRAM_POINT_SIZE);
    if (m_showPoints)
        m_allPoints.drawPoints(f, pointShader, vp, 10.0f);
    m_socketPoints.drawPoints(f, pointShader, vp, 9.0f);
    m_selBatch.drawPoints(f, pointShader, vp, 14.0f);
    m_selSockets.drawPoints(f, pointShader, vp, 15.0f);
    f->glDisable(GL_PROGRAM_POINT_SIZE);
}

void RoadRenderer::destroy(QOpenGLFunctions_4_1_Core* f) {
    m_roads.destroy(f);
    m_nodes.destroy(f);
    m_socketLines.destroy(f);
    m_socketPoints.destroy(f);
    m_allPoints.destroy(f);
    m_allCtrlLines.destroy(f);
    m_selBatch.destroy(f);
    m_selRoad.destroy(f);
    m_selSockets.destroy(f);
    m_surfaceMesh.destroy(f);
    m_intersectionMesh.destroy(f);
    m_ready   = false;
    m_hasData = false;
}
