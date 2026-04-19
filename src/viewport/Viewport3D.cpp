#include "Viewport3D.h"
#include "BuildConfig.h"
#include "../generator/ClothoidGen.h"
#include "../model/Serializer.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>

Viewport3D::Viewport3D(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    connect(&m_timer, &QTimer::timeout, this, QOverload<>::of(&QOpenGLWidget::update));
    m_timer.start(16); // ~60 fps
}

Viewport3D::~Viewport3D() {
    makeCurrent();
    m_grid.destroy(this);
    m_axisGizmo.destroy(this);
    m_roadRenderer.destroy(this);
    m_transformGizmo.destroy(this);
    doneCurrent();
}

void Viewport3D::initializeGL() {
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

    QString outDir = QCoreApplication::applicationDirPath() + "/shaders/";
    QString sourceDir = QStringLiteral(ROAD_EDITOR_SOURCE_DIR) + "/shaders/";
    QString shaderDir = QDir(outDir).exists() ? outDir : sourceDir;

    if (!m_lineShader.load(shaderDir + "line.vert", shaderDir + "line.frag")) {
        qWarning() << "Line shader load failed - check shaders/ directory";
    }
    if (!m_roadShader.load(shaderDir + "road.vert", shaderDir + "road.frag")) {
        qWarning() << "Road shader load failed - check shaders/ directory";
    }
    if (!m_pointShader.load(shaderDir + "point.vert", shaderDir + "point.frag")) {
        qWarning() << "Point shader load failed - check shaders/ directory";
    }

    m_grid.init(this);
    m_axisGizmo.init(this);
    m_roadRenderer.init(this);
    m_transformGizmo.init(this);
    m_glReady = true;

    if (!m_pendingPath.isEmpty()) {
        if (Serializer::loadFromFile(m_pendingPath, m_network)) {
            m_roadRenderer.rebuild(this, m_network);
        }
        m_pendingPath.clear();
    }
}

void Viewport3D::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    m_aspect = (h > 0) ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;
}

void Viewport3D::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = m_camera.viewMatrix();
    glm::mat4 vp   = m_camera.projMatrix(m_aspect) * view;
    m_grid.draw(this, m_lineShader, vp);
    m_roadRenderer.draw(this, m_lineShader, m_roadShader, m_pointShader, vp);

    // Transform gizmo — drawn over road geometry, depth-test disabled
    if (m_glReady && m_editor.mode == ToolMode::Edit) {
        glm::vec3 glPos;
        bool showGizmo = false;
        if (m_editor.sel.roadIdx >= 0 && m_editor.sel.pointIdx >= 0 &&
            m_editor.sel.roadIdx < (int)m_network.roads.size()) {
            const auto& pt = m_network.roads[m_editor.sel.roadIdx]
                                       .points[m_editor.sel.pointIdx];
            glPos = {-pt.pos.x, pt.pos.y, pt.pos.z};
            showGizmo = true;
        } else if (m_editor.sel.intersectionIdx >= 0 &&
                   m_editor.sel.intersectionIdx < (int)m_network.intersections.size()) {
            const auto& ix = m_network.intersections[m_editor.sel.intersectionIdx];
            glPos = {-ix.pos.x, ix.pos.y, ix.pos.z};
            showGizmo = true;
        }
        if (showGizmo) {
            m_transformGizmo.rebuild(this, glPos, m_camera.position(), m_gizmoHover);
            glDisable(GL_DEPTH_TEST);
            m_transformGizmo.draw(this, m_lineShader, vp);
            glEnable(GL_DEPTH_TEST);
        }
    }

    m_axisGizmo.draw(this, m_lineShader, view, width(), height());
}

void Viewport3D::setToolMode(ToolMode m) {
    m_editor.mode = m;
    m_roadRenderer.setShowPoints(m == ToolMode::Edit);
    update();
}

void Viewport3D::setWireframe(bool on) {
    m_roadRenderer.setWireframe(on);
    update();
}


void Viewport3D::applyRoadProperties(int roadIdx, RoadProperties p) {
    if (roadIdx < 0 || roadIdx >= static_cast<int>(m_network.roads.size())) return;
    m_editor.pushUndo(m_network);
    auto& road = m_network.roads[roadIdx];
    road.defaultTargetSpeed      = p.speed;
    road.useLaneLeft2            = p.useLaneLeft2;
    road.defaultWidthLaneLeft2   = p.widthLaneLeft2;
    road.useLaneLeft1            = p.useLaneLeft1;
    road.defaultWidthLaneLeft1   = p.widthLaneLeft1;
    road.useLaneCenter           = p.useLaneCenter;
    road.defaultWidthLaneCenter  = p.widthLaneCenter;
    road.useLaneRight1           = p.useLaneRight1;
    road.defaultWidthLaneRight1  = p.widthLaneRight1;
    road.useLaneRight2           = p.useLaneRight2;
    road.defaultWidthLaneRight2  = p.widthLaneRight2;
    road.segmentLength           = std::max(p.segmentLength, 0.01f);
    road.equalMidpoint           = p.equalMidpoint;
    makeCurrent();
    m_roadRenderer.rebuild(this, m_network);
    doneCurrent();
    emit networkChanged();
}

void Viewport3D::loadNetwork(const QString& path) {
    if (!m_glReady) {
        m_pendingPath = path;
        return;
    }

    makeCurrent();
    if (Serializer::loadFromFile(path, m_network)) {
        m_roadRenderer.rebuild(this, m_network);
    }
    doneCurrent();
    emit networkChanged();
    emit selectionChanged(-1);
    update();
}

// Ray casting helpers

glm::vec3 Viewport3D::screenToRay(const QPoint& p) const {
    float ndcX = (2.0f * p.x()) / width() - 1.0f;
    float ndcY = -(2.0f * p.y()) / height() + 1.0f;

    glm::mat4 proj = m_camera.projMatrix(m_aspect);
    glm::mat4 view = m_camera.viewMatrix();
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 near4 = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 far4 = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    glm::vec3 nearP = glm::vec3(near4) / near4.w;
    glm::vec3 farP = glm::vec3(far4) / far4.w;
    return glm::normalize(farP - nearP);
}

glm::vec3 Viewport3D::rayHitY(const glm::vec3& origin, const glm::vec3& dir, float y) const {
    float t = (std::abs(dir.y) > 1e-6f) ? (y - origin.y) / dir.y : 0.0f;
    return origin + dir * t;
}

int Viewport3D::pickRoad(const QPoint& screenPos) const {
    const float kPickRadiusSq = 12.0f * 12.0f; // pixels
    float bestDist = std::numeric_limits<float>::max();
    int bestRoad = -1;

    glm::mat4 vp = m_camera.projMatrix(m_aspect) * m_camera.viewMatrix();
    glm::vec2 mp = { float(screenPos.x()), float(screenPos.y()) };

    auto project = [&](glm::vec3 worldPos) -> std::pair<glm::vec2, bool> {
        glm::vec4 clip = vp * glm::vec4(-worldPos.x, worldPos.y, worldPos.z, 1.0f);
        if (clip.w <= 0.0f) return {{}, false};
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        return {{ (ndc.x + 1.0f) * 0.5f * width(),
                  (1.0f - ndc.y) * 0.5f * height() }, true};
    };

    for (int ri = 0; ri < static_cast<int>(m_network.roads.size()); ++ri) {
        const auto& road = m_network.roads[ri];
        for (size_t i = 0; i + 1 < road.points.size(); ++i) {
            auto [sa, okA] = project(road.points[i    ].pos);
            auto [sb, okB] = project(road.points[i + 1].pos);
            if (!okA || !okB) continue;

            // Distance from mp to segment sa→sb
            glm::vec2 ab = sb - sa;
            float lenSq = glm::dot(ab, ab);
            float distSq;
            if (lenSq < 1e-6f) {
                glm::vec2 d = mp - sa;
                distSq = glm::dot(d, d);
            } else {
                float t = std::clamp(glm::dot(mp - sa, ab) / lenSq, 0.0f, 1.0f);
                glm::vec2 proj = sa + t * ab;
                glm::vec2 d = mp - proj;
                distSq = glm::dot(d, d);
            }

            if (distSq < kPickRadiusSq && distSq < bestDist) {
                bestDist = distSq;
                bestRoad = ri;
            }
        }
    }
    return bestRoad;
}

int Viewport3D::pickIntersection(const QPoint& screenPos) const {
    const float kPickRadiusSq = 20.0f * 20.0f;
    float bestDist = std::numeric_limits<float>::max();
    int best = -1;

    glm::mat4 vp = m_camera.projMatrix(m_aspect) * m_camera.viewMatrix();
    glm::vec2 mp = { float(screenPos.x()), float(screenPos.y()) };

    for (int i = 0; i < (int)m_network.intersections.size(); ++i) {
        const auto& ix = m_network.intersections[i];
        glm::vec4 clip = vp * glm::vec4(-ix.pos.x, ix.pos.y, ix.pos.z, 1.0f);
        if (clip.w <= 0.0f) continue;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        float sx = (ndc.x + 1.0f) * 0.5f * width();
        float sy = (1.0f - ndc.y) * 0.5f * height();
        float dx = sx - mp.x, dy = sy - mp.y;
        float distSq = dx*dx + dy*dy;
        if (distSq < kPickRadiusSq && distSq < bestDist) {
            bestDist = distSq;
            best = i;
        }
    }
    return best;
}

void Viewport3D::setupIxDragEndpoints(int ixIdx) {
    m_ixDragEndpoints.clear();
    if (ixIdx < 0 || ixIdx >= (int)m_network.intersections.size()) return;
    const auto& ix = m_network.intersections[ixIdx];
    for (int ri = 0; ri < (int)m_network.roads.size(); ++ri) {
        const auto& road = m_network.roads[ri];
        if (road.active == 0 || road.points.empty()) continue;
        if (road.startIntersectionId == ix.id)
            m_ixDragEndpoints.push_back({ri, 0,
                road.points.front().pos - ix.pos});
        if (road.endIntersectionId == ix.id)
            m_ixDragEndpoints.push_back({ri, (int)road.points.size() - 1,
                road.points.back().pos - ix.pos});
    }
}

bool Viewport3D::pickControlPoint(
    const glm::vec3& rayOrigin, const glm::vec3& rayDir, int& outRoadIdx, int& outPointIdx) {
    Q_UNUSED(rayOrigin);
    Q_UNUSED(rayDir);

    const float kPickRadiusSq = 20.0f * 20.0f;
    float bestDist = std::numeric_limits<float>::max();
    outRoadIdx = -1;
    outPointIdx = -1;

    glm::mat4 proj = m_camera.projMatrix(m_aspect);
    glm::mat4 view = m_camera.viewMatrix();
    glm::mat4 vp = proj * view;

    for (int ri = 0; ri < static_cast<int>(m_network.roads.size()); ++ri) {
        const auto& road = m_network.roads[ri];
        for (int pi = 0; pi < static_cast<int>(road.points.size()); ++pi) {
            glm::vec3 glPos = {
                -road.points[pi].pos.x,
                road.points[pi].pos.y,
                road.points[pi].pos.z,
            };

            glm::vec4 clip = vp * glm::vec4(glPos, 1.0f);
            if (clip.w <= 0.0f) {
                continue;
            }

            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            float sx = (ndc.x + 1.0f) * 0.5f * width();
            float sy = (1.0f - ndc.y) * 0.5f * height();

            float dx = sx - m_lastPos.x();
            float dy = sy - m_lastPos.y();
            float distSq = dx * dx + dy * dy;

            if (distSq < kPickRadiusSq && distSq < bestDist) {
                bestDist = distSq;
                outRoadIdx = ri;
                outPointIdx = pi;
            }
        }
    }

    return outRoadIdx >= 0;
}

// Mouse events

void Viewport3D::mousePressEvent(QMouseEvent* e) {
    m_lastPos = e->pos();

    if (e->modifiers() & Qt::AltModifier) {
        if (e->button() == Qt::LeftButton) {
            m_rotating = true;
        } else if (e->button() == Qt::MiddleButton) {
            m_panning = true;
        }
        return;
    }

    if (e->button() == Qt::LeftButton && m_glReady) {
        glm::vec3 rayDir = screenToRay(e->pos());
        glm::vec3 rayOri = m_camera.position();

        if (m_editor.mode == ToolMode::Select) {
            // Select mode: pick road by edge proximity, no dragging
            int ri = pickRoad(e->pos());
            if (ri >= 0) {
                if (m_editor.sel.roadIdx != ri) {
                    m_editor.sel.roadIdx  = ri;
                    m_editor.sel.pointIdx = -1;
                    makeCurrent();
                    m_roadRenderer.updateSelection(this, m_network, ri, -1);
                    doneCurrent();
                    emit selectionChanged(ri);
                }
            } else {
                m_editor.sel.clear();
                makeCurrent();
                m_roadRenderer.updateSelection(this, m_network, -1, -1);
                doneCurrent();
                emit selectionChanged(-1);
            }
            m_dragging = false;
        } else {
            // Edit mode: resolve gizmo position from whatever is currently selected
            {
                glm::vec3 glPos;
                bool hasGizmo = false;
                if (m_editor.sel.roadIdx >= 0 && m_editor.sel.pointIdx >= 0 &&
                    m_editor.sel.roadIdx < (int)m_network.roads.size()) {
                    const auto& pt = m_network.roads[m_editor.sel.roadIdx]
                                               .points[m_editor.sel.pointIdx];
                    glPos = {-pt.pos.x, pt.pos.y, pt.pos.z};
                    hasGizmo = true;
                } else if (m_editor.sel.intersectionIdx >= 0 &&
                           m_editor.sel.intersectionIdx < (int)m_network.intersections.size()) {
                    const auto& ix = m_network.intersections[m_editor.sel.intersectionIdx];
                    glPos = {-ix.pos.x, ix.pos.y, ix.pos.z};
                    hasGizmo = true;
                }

                if (hasGizmo) {
                    glm::mat4 vpMat = m_camera.projMatrix(m_aspect) * m_camera.viewMatrix();
                    auto axis = m_transformGizmo.hitTest(
                        e->pos(), glPos, m_camera.position(), vpMat, width(), height());

                    if (axis != TransformGizmo::Axis::None) {
                        m_gizmoDragAxis      = axis;
                        m_gizmoDragOrigGlPos = glPos;

                        if (axis == TransformGizmo::Axis::Screen) {
                            glm::vec3 planeNorm = glm::normalize(m_camera.position() - glPos);
                            float denom = glm::dot(rayDir, planeNorm);
                            if (std::abs(denom) > 1e-6f) {
                                float t = glm::dot(glPos - rayOri, planeNorm) / denom;
                                m_gizmoDragScreenHit0 = rayOri + t * rayDir;
                            } else {
                                m_gizmoDragScreenHit0 = glPos;
                            }
                        } else {
                            m_gizmoDragT0 = TransformGizmo::axisTParam(
                                rayOri, rayDir, glPos, TransformGizmo::axisDir(axis));
                        }

                        if (m_editor.sel.hasIntersection())
                            setupIxDragEndpoints(m_editor.sel.intersectionIdx);

                        m_dragging = true;
                        m_editor.pushUndo(m_network);
                        return;
                    }
                }
            }

            // Try picking an intersection center
            {
                int ixIdx = pickIntersection(e->pos());
                if (ixIdx >= 0) {
                    m_editor.sel.clear();
                    m_editor.sel.intersectionIdx = ixIdx;
                    makeCurrent();
                    m_roadRenderer.updateSelection(this, m_network, -1, -1);
                    doneCurrent();
                    emit selectionChanged(-1);
                    const auto& ix = m_network.intersections[ixIdx];
                    glm::vec3 glPos = {-ix.pos.x, ix.pos.y, ix.pos.z};
                    m_gizmoDragAxis = TransformGizmo::Axis::None;
                    m_dragging      = true;
                    m_dragPlaneY    = glPos;
                    setupIxDragEndpoints(ixIdx);
                    m_editor.pushUndo(m_network);
                    return;
                }
            }

            // Pick control point; fall back to road selection
            int ri = -1, pi = -1;
            if (pickControlPoint(rayOri, rayDir, ri, pi)) {
                bool roadChanged = m_editor.sel.roadIdx  != ri;
                bool ptChanged   = m_editor.sel.pointIdx != pi;
                if (roadChanged || ptChanged) {
                    m_editor.sel.roadIdx  = ri;
                    m_editor.sel.pointIdx = pi;
                    makeCurrent();
                    m_roadRenderer.updateSelection(this, m_network, ri, pi);
                    doneCurrent();
                    emit selectionChanged(ri);
                }
                glm::vec3 glPos = {
                    -m_network.roads[ri].points[pi].pos.x,
                     m_network.roads[ri].points[pi].pos.y,
                     m_network.roads[ri].points[pi].pos.z,
                };
                m_dragging   = true;
                m_dragPlaneY = glPos;
                m_editor.pushUndo(m_network);
            } else {
                // No vertex hit — try road edge for selection only
                ri = pickRoad(e->pos());
                if (ri >= 0) {
                    if (m_editor.sel.roadIdx != ri) {
                        m_editor.sel.roadIdx  = ri;
                        m_editor.sel.pointIdx = -1;
                        makeCurrent();
                        m_roadRenderer.updateSelection(this, m_network, ri, -1);
                        doneCurrent();
                        emit selectionChanged(ri);
                    }
                } else {
                    m_editor.sel.clear();
                    makeCurrent();
                    m_roadRenderer.updateSelection(this, m_network, -1, -1);
                    doneCurrent();
                    emit selectionChanged(-1);
                }
                m_dragging = false;
            }
        }
    }
}

void Viewport3D::mouseMoveEvent(QMouseEvent* e) {
    QPoint delta = e->pos() - m_lastPos;
    m_lastPos = e->pos();

    if (m_rotating) {
        m_camera.orbit(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
    }
    if (m_panning) {
        m_camera.pan(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
    }

    if (m_dragging && !(e->modifiers() & Qt::AltModifier)) {
        glm::vec3 rayDir = screenToRay(e->pos());
        glm::vec3 rayOri = m_camera.position();

        auto resolveGlPos = [&]() -> glm::vec3 {
            if (m_gizmoDragAxis == TransformGizmo::Axis::Screen) {
                glm::vec3 planeNorm = glm::normalize(m_camera.position() - m_gizmoDragOrigGlPos);
                float denom = glm::dot(rayDir, planeNorm);
                if (std::abs(denom) > 1e-6f) {
                    float t = glm::dot(m_gizmoDragOrigGlPos - rayOri, planeNorm) / denom;
                    return m_gizmoDragOrigGlPos + (rayOri + t * rayDir - m_gizmoDragScreenHit0);
                }
                return m_gizmoDragOrigGlPos;
            } else if (m_gizmoDragAxis != TransformGizmo::Axis::None) {
                glm::vec3 axDir = TransformGizmo::axisDir(m_gizmoDragAxis);
                float t = TransformGizmo::axisTParam(rayOri, rayDir, m_gizmoDragOrigGlPos, axDir);
                return m_gizmoDragOrigGlPos + (t - m_gizmoDragT0) * axDir;
            } else {
                return rayHitY(rayOri, rayDir, m_dragPlaneY.y);
            }
        };

        if (m_editor.sel.valid()) {
            // Drag control point
            int ri = m_editor.sel.roadIdx;
            int pi = m_editor.sel.pointIdx;
            glm::vec3 newGlPos = resolveGlPos();
            m_network.roads[ri].points[pi].pos = {-newGlPos.x, newGlPos.y, newGlPos.z};
            makeCurrent();
            m_roadRenderer.rebuild(this, m_network);
            m_roadRenderer.updateSelection(this, m_network, ri, pi);
            doneCurrent();
            emit networkChanged();
        } else if (m_editor.sel.hasIntersection()) {
            // Drag intersection + connected road endpoints
            int ixIdx = m_editor.sel.intersectionIdx;
            if (ixIdx >= 0 && ixIdx < (int)m_network.intersections.size()) {
                glm::vec3 newGlPos  = resolveGlPos();
                glm::vec3 newWorld  = {-newGlPos.x, newGlPos.y, newGlPos.z};
                m_network.intersections[ixIdx].pos = newWorld;
                for (const auto& ep : m_ixDragEndpoints)
                    m_network.roads[ep.roadIdx].points[ep.ptIdx].pos = newWorld + ep.origOffset;
                makeCurrent();
                m_roadRenderer.rebuild(this, m_network);
                m_roadRenderer.updateSelection(this, m_network, -1, -1);
                doneCurrent();
                emit networkChanged();
            }
        }
    }

    // Update gizmo hover highlight when not dragging
    if (!m_dragging && m_glReady && m_editor.mode == ToolMode::Edit) {
        glm::vec3 glPos;
        bool hasGizmo = false;
        if (m_editor.sel.roadIdx >= 0 && m_editor.sel.pointIdx >= 0 &&
            m_editor.sel.roadIdx < (int)m_network.roads.size()) {
            const auto& pt = m_network.roads[m_editor.sel.roadIdx]
                                       .points[m_editor.sel.pointIdx];
            glPos = {-pt.pos.x, pt.pos.y, pt.pos.z};
            hasGizmo = true;
        } else if (m_editor.sel.intersectionIdx >= 0 &&
                   m_editor.sel.intersectionIdx < (int)m_network.intersections.size()) {
            const auto& ix = m_network.intersections[m_editor.sel.intersectionIdx];
            glPos = {-ix.pos.x, ix.pos.y, ix.pos.z};
            hasGizmo = true;
        }
        if (hasGizmo) {
            glm::mat4 vpMat = m_camera.projMatrix(m_aspect) * m_camera.viewMatrix();
            auto newHover = m_transformGizmo.hitTest(
                e->pos(), glPos, m_camera.position(), vpMat, width(), height());
            if (newHover != m_gizmoHover) { m_gizmoHover = newHover; update(); }
        } else if (m_gizmoHover != TransformGizmo::Axis::None) {
            m_gizmoHover = TransformGizmo::Axis::None;
            update();
        }
    }
}

void Viewport3D::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging       = false;
        m_gizmoDragAxis  = TransformGizmo::Axis::None;
    }
    m_rotating = false;
    m_panning  = false;
}

void Viewport3D::wheelEvent(QWheelEvent* e) {
    if (!m_altDown) {
        e->ignore();
        return;
    }

    const QPoint angleDelta = e->angleDelta();
    const int dominantDelta = (angleDelta.y() != 0) ? angleDelta.y() : angleDelta.x();
    const float delta = static_cast<float>(dominantDelta) / 120.0f;
    m_camera.zoom(delta);
    e->accept();
}

void Viewport3D::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Alt) {
        m_altDown = true;
        return;
    }
    if (!m_glReady) {
        return;
    }

    if (e->modifiers() & Qt::ControlModifier) {
        if (e->key() == Qt::Key_Z) {
            if (m_editor.undo(m_network)) {
                makeCurrent();
                m_roadRenderer.rebuild(this, m_network);
                m_roadRenderer.updateSelection(
                    this, m_network, m_editor.sel.roadIdx, m_editor.sel.pointIdx);
                doneCurrent();
            }
            return;
        }
        if (e->key() == Qt::Key_Y) {
            if (m_editor.redo(m_network)) {
                makeCurrent();
                m_roadRenderer.rebuild(this, m_network);
                m_roadRenderer.updateSelection(
                    this, m_network, m_editor.sel.roadIdx, m_editor.sel.pointIdx);
                doneCurrent();
            }
            return;
        }
    }

    QOpenGLWidget::keyPressEvent(e);
}

void Viewport3D::keyReleaseEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Alt) {
        m_altDown = false;
    }
    QOpenGLWidget::keyReleaseEvent(e);
}
