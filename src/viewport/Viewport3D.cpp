#include "Viewport3D.h"
#include "BuildConfig.h"
#include "../generator/ClothoidGen.h"
#include "../model/Serializer.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QStringConverter>
#include <QTextStream>
#include <QTime>
#include <QWheelEvent>
#include <set>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>

namespace {
constexpr int kBoxSelectThresholdPx = 4;
constexpr int kScreenHandlePriorityRadiusPx = 26;

const char* axisName(TransformGizmo::Axis axis) {
    switch (axis) {
    case TransformGizmo::Axis::X: return "X";
    case TransformGizmo::Axis::Y: return "Y";
    case TransformGizmo::Axis::Z: return "Z";
    case TransformGizmo::Axis::Screen: return "Screen";
    default: return "None";
    }
}

bool isShiftHeld(Qt::KeyboardModifiers mods) {
    return mods.testFlag(Qt::ShiftModifier) ||
           QApplication::keyboardModifiers().testFlag(Qt::ShiftModifier);
}

QString inputDebugLogPath() {
    return QDir(QStringLiteral(ROAD_EDITOR_SOURCE_DIR)).filePath("input_debug.log");
}

void appendInputDebugLog(const QString& message) {
    QFile file{inputDebugLogPath()};
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Encoding::Utf8);
    out << QTime::currentTime().toString("HH:mm:ss.zzz")
        << " " << message << "\n";
}
}

Viewport3D::Viewport3D(QWidget* parent)
    : QOpenGLWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    m_boxRubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    connect(&m_timer, &QTimer::timeout, this, QOverload<>::of(&QOpenGLWidget::update));
    m_timer.start(16);
    appendInputDebugLog("Viewport3D constructed");
}

Viewport3D::~Viewport3D() {
    makeCurrent();
    m_grid.destroy(this);
    m_axisGizmo.destroy(this);
    m_boxOverlay.destroy(this);
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

    if (!m_lineShader.load(shaderDir + "line.vert", shaderDir + "line.frag"))
        qWarning() << "Line shader load failed - check shaders/ directory";
    if (!m_roadShader.load(shaderDir + "road.vert", shaderDir + "road.frag"))
        qWarning() << "Road shader load failed - check shaders/ directory";
    if (!m_pointShader.load(shaderDir + "point.vert", shaderDir + "point.frag"))
        qWarning() << "Point shader load failed - check shaders/ directory";

    m_grid.init(this);
    m_axisGizmo.init(this);
    m_boxOverlay.init(this);
    m_roadRenderer.init(this);
    m_transformGizmo.init(this);
    m_glReady = true;

    if (!m_pendingPath.isEmpty()) {
        if (Serializer::loadFromFile(m_pendingPath, m_network))
            m_roadRenderer.rebuild(this, m_network);
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

    if (m_glReady && m_editor.mode == ToolMode::Edit) {
        const bool showGizmo = m_editor.sel.valid() || m_editor.sel.hasIntersection();
        if (showGizmo) {
            glm::vec3 glPos = selectionPivotGlPos();
            m_transformGizmo.rebuild(this, glPos, cameraGlPos(), m_gizmoHover);
            glDisable(GL_DEPTH_TEST);
            m_transformGizmo.draw(this, m_lineShader, vp);
            glEnable(GL_DEPTH_TEST);
        }
    }

    if (m_boxSelectPending || m_boxSelecting) {
        auto toNdc = [&](const QPoint& p) -> glm::vec3 {
            float x = (2.0f * static_cast<float>(p.x())) / static_cast<float>(width()) - 1.0f;
            float y = 1.0f - (2.0f * static_cast<float>(p.y())) / static_cast<float>(height());
            return {x, y, 0.0f};
        };

        QRect rect = QRect(m_boxSelectOrigin, m_boxSelectCurrent).normalized();
        glm::vec3 color = {0.25f, 0.8f, 1.0f};
        glm::vec3 a = toNdc(rect.topLeft());
        glm::vec3 b = toNdc(QPoint(rect.right(), rect.top()));
        glm::vec3 c = toNdc(rect.bottomRight());
        glm::vec3 d = toNdc(QPoint(rect.left(), rect.bottom()));

        m_boxOverlay.begin();
        m_boxOverlay.addLine(a, b, color);
        m_boxOverlay.addLine(b, c, color);
        m_boxOverlay.addLine(c, d, color);
        m_boxOverlay.addLine(d, a, color);
        m_boxOverlay.upload(this);

        glDisable(GL_DEPTH_TEST);
        m_boxOverlay.draw(this, m_lineShader, glm::mat4(1.0f));
        glEnable(GL_DEPTH_TEST);
    }

    m_axisGizmo.draw(this, m_lineShader, view, width(), height());
}

void Viewport3D::setToolMode(ToolMode m) {
    m_editor.mode = m;
    m_roadRenderer.setShowPoints(m == ToolMode::Edit);
    m_boxSelectPending = false;
    m_boxSelecting = false;
    m_boxSelectRoadCandidate = -1;
    m_boxRubberBand->hide();
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
    syncSelectionVisuals();
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
        m_editor.sel.clear();
        m_roadRenderer.rebuild(this, m_network);
        syncSelectionVisuals();
    }
    doneCurrent();
    emit networkChanged();
    emit selectionChanged(-1);
    update();
}

glm::vec3 Viewport3D::screenToRay(const QPoint& p) const {
    float ndcX = (2.0f * p.x()) / width() - 1.0f;
    float ndcY = -(2.0f * p.y()) / height() + 1.0f;

    glm::mat4 proj = m_camera.projMatrix(m_aspect);
    glm::mat4 view = m_camera.viewMatrix();
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 near4 = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 far4  = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    glm::vec3 nearP = glm::vec3(near4) / near4.w;
    glm::vec3 farP  = glm::vec3(far4) / far4.w;
    return glm::normalize(farP - nearP);
}

glm::vec3 Viewport3D::screenToGlAtDepth(const QPoint& p, float ndcZ) const {
    float ndcX = (2.0f * p.x()) / width() - 1.0f;
    float ndcY = -(2.0f * p.y()) / height() + 1.0f;

    glm::mat4 proj = m_camera.projMatrix(m_aspect);
    glm::mat4 view = m_camera.viewMatrix();
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 world4 = invVP * glm::vec4(ndcX, ndcY, ndcZ, 1.0f);
    return glm::vec3(world4) / world4.w;
}

glm::vec3 Viewport3D::rayHitY(const glm::vec3& origin, const glm::vec3& dir, float y) const {
    float t = (std::abs(dir.y) > 1e-6f) ? (y - origin.y) / dir.y : 0.0f;
    return origin + dir * t;
}

glm::vec3 Viewport3D::cameraForwardWorld() const {
    const QPoint center(width() / 2, height() / 2);
    return screenToRay(center);
}

glm::vec3 Viewport3D::cameraForwardGl() const {
    return toGlVector(cameraForwardWorld());
}

glm::vec3 Viewport3D::cameraGlPos() const {
    glm::vec3 camPos = m_camera.position();
    return {-camPos.x, camPos.y, camPos.z};
}

glm::vec3 Viewport3D::toGlVector(const glm::vec3& v) const {
    return {-v.x, v.y, v.z};
}

glm::vec3 Viewport3D::selectionPivotGlPos() const {
    if (m_editor.sel.valid()) {
        glm::vec3 sum(0.0f);
        int count = 0;
        for (const auto& selPt : m_editor.sel.points) {
            if (selPt.roadIdx < 0 || selPt.roadIdx >= (int)m_network.roads.size()) continue;
            if (selPt.pointIdx < 0 || selPt.pointIdx >= (int)m_network.roads[selPt.roadIdx].points.size()) continue;
            const auto& pos = m_network.roads[selPt.roadIdx].points[selPt.pointIdx].pos;
            sum += glm::vec3(-pos.x, pos.y, pos.z);
            ++count;
        }
        if (count > 0) return sum / static_cast<float>(count);
    }

    if (m_editor.sel.hasSocket() &&
        m_editor.sel.intersectionIdx < (int)m_network.intersections.size()) {
        const auto& ix = m_network.intersections[m_editor.sel.intersectionIdx];
        if (m_editor.sel.socketIdx >= 0 && m_editor.sel.socketIdx < (int)ix.sockets.size()) {
            const auto& socket = ix.sockets[m_editor.sel.socketIdx];
            glm::vec3 pos = ix.pos + socket.localPos;
            return {-pos.x, pos.y, pos.z};
        }
    }

    if (m_editor.sel.hasIntersection() &&
        m_editor.sel.intersectionIdx < (int)m_network.intersections.size()) {
        const auto& ix = m_network.intersections[m_editor.sel.intersectionIdx];
        return {-ix.pos.x, ix.pos.y, ix.pos.z};
    }

    return {0.0f, 0.0f, 0.0f};
}

int Viewport3D::selectedRoadForPanels() const {
    if (m_editor.sel.hasIntersection()) return -1;
    if (m_editor.sel.points.empty()) return m_editor.sel.roadIdx;

    int roadIdx = m_editor.sel.points.front().roadIdx;
    for (const auto& pt : m_editor.sel.points)
        if (pt.roadIdx != roadIdx)
            return -1;
    return roadIdx;
}

void Viewport3D::syncSelectionVisuals() {
    if (!m_glReady) return;
    m_roadRenderer.updateSelection(this, m_network, m_editor.sel);
    emit selectionChanged(selectedRoadForPanels());
    emit selectionStateChanged(m_editor.sel);
}

void Viewport3D::beginPointDrag(const glm::vec3& pivotGlPos) {
    m_pointDragOrigins.clear();
    for (const auto& selPt : m_editor.sel.points) {
        if (selPt.roadIdx < 0 || selPt.roadIdx >= (int)m_network.roads.size()) continue;
        if (selPt.pointIdx < 0 || selPt.pointIdx >= (int)m_network.roads[selPt.roadIdx].points.size()) continue;
        m_pointDragOrigins.push_back(
            {selPt, m_network.roads[selPt.roadIdx].points[selPt.pointIdx].pos});
    }

    m_dragPlaneY = pivotGlPos;
    m_dragging = !m_pointDragOrigins.empty();
    if (m_dragging)
        m_editor.pushUndo(m_network);
}

void Viewport3D::applySelectedSocketProperties(const QString& name, float yaw, bool enabled) {
    if (!m_editor.sel.hasSocket()) return;
    if (m_editor.sel.intersectionIdx < 0 ||
        m_editor.sel.intersectionIdx >= (int)m_network.intersections.size())
        return;
    auto& ix = m_network.intersections[m_editor.sel.intersectionIdx];
    if (m_editor.sel.socketIdx < 0 || m_editor.sel.socketIdx >= (int)ix.sockets.size())
        return;

    m_editor.pushUndo(m_network);
    auto& socket = ix.sockets[m_editor.sel.socketIdx];
    socket.name = name.toStdString();
    socket.yaw = yaw;
    socket.enabled = enabled;
    syncLinkedEndpointsForIntersection(m_editor.sel.intersectionIdx);
    makeCurrent();
    m_roadRenderer.rebuild(this, m_network);
    syncSelectionVisuals();
    doneCurrent();
    emit networkChanged();
}

void Viewport3D::addSocketToSelectedIntersection() {
    if (m_editor.sel.intersectionIdx < 0 ||
        m_editor.sel.intersectionIdx >= (int)m_network.intersections.size())
        return;

    auto& ix = m_network.intersections[m_editor.sel.intersectionIdx];
    m_editor.pushUndo(m_network);
    int index = static_cast<int>(ix.sockets.size());
    float radius = std::max(ix.entryDist, 12.0f);
    float angle = index * 0.78539816339f;
    IntersectionSocket socket;
    socket.id = "s" + std::to_string(index);
    socket.name = "Socket " + std::to_string(index + 1);
    socket.localPos = {std::cos(angle) * radius, 0.0f, std::sin(angle) * radius};
    socket.yaw = angle;
    socket.enabled = true;
    ix.sockets.push_back(socket);
    m_editor.sel.setIntersection(m_editor.sel.intersectionIdx, index);
    makeCurrent();
    m_roadRenderer.rebuild(this, m_network);
    syncSelectionVisuals();
    doneCurrent();
    emit networkChanged();
}

void Viewport3D::removeSelectedSocket() {
    if (!m_editor.sel.hasSocket()) return;
    if (m_editor.sel.intersectionIdx < 0 ||
        m_editor.sel.intersectionIdx >= (int)m_network.intersections.size())
        return;

    auto& ix = m_network.intersections[m_editor.sel.intersectionIdx];
    if (m_editor.sel.socketIdx < 0 || m_editor.sel.socketIdx >= (int)ix.sockets.size())
        return;

    const std::string removedSocketId = ix.sockets[m_editor.sel.socketIdx].id;
    m_editor.pushUndo(m_network);
    for (auto& road : m_network.roads) {
        if (road.startLink.intersectionId == ix.id && road.startLink.socketId == removedSocketId) {
            road.startLink.clear();
            road.startIntersectionId.clear();
        }
        if (road.endLink.intersectionId == ix.id && road.endLink.socketId == removedSocketId) {
            road.endLink.clear();
            road.endIntersectionId.clear();
        }
    }
    ix.sockets.erase(ix.sockets.begin() + m_editor.sel.socketIdx);
    if (!ix.sockets.empty())
        m_editor.sel.setIntersection(m_editor.sel.intersectionIdx,
                                     std::min(m_editor.sel.socketIdx, (int)ix.sockets.size() - 1));
    else
        m_editor.sel.setIntersection(m_editor.sel.intersectionIdx, -1);
    makeCurrent();
    m_roadRenderer.rebuild(this, m_network);
    syncSelectionVisuals();
    doneCurrent();
    emit networkChanged();
}

int Viewport3D::pickRoad(const QPoint& screenPos) const {
    const float kPickRadiusSq = 12.0f * 12.0f;
    float bestDist = std::numeric_limits<float>::max();
    int bestRoad = -1;

    glm::mat4 vp = m_camera.projMatrix(m_aspect) * m_camera.viewMatrix();
    glm::vec2 mp = {float(screenPos.x()), float(screenPos.y())};

    auto project = [&](glm::vec3 worldPos) -> std::pair<glm::vec2, bool> {
        glm::vec4 clip = vp * glm::vec4(-worldPos.x, worldPos.y, worldPos.z, 1.0f);
        if (clip.w <= 0.0f) return {{}, false};
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        return {{(ndc.x + 1.0f) * 0.5f * width(),
                 (1.0f - ndc.y) * 0.5f * height()}, true};
    };

    for (int ri = 0; ri < static_cast<int>(m_network.roads.size()); ++ri) {
        const auto& road = m_network.roads[ri];
        for (size_t i = 0; i + 1 < road.points.size(); ++i) {
            auto [sa, okA] = project(road.points[i].pos);
            auto [sb, okB] = project(road.points[i + 1].pos);
            if (!okA || !okB) continue;

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
    glm::vec2 mp = {float(screenPos.x()), float(screenPos.y())};

    for (int i = 0; i < (int)m_network.intersections.size(); ++i) {
        const auto& ix = m_network.intersections[i];
        glm::vec4 clip = vp * glm::vec4(-ix.pos.x, ix.pos.y, ix.pos.z, 1.0f);
        if (clip.w <= 0.0f) continue;
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        float sx = (ndc.x + 1.0f) * 0.5f * width();
        float sy = (1.0f - ndc.y) * 0.5f * height();
        float dx = sx - mp.x;
        float dy = sy - mp.y;
        float distSq = dx * dx + dy * dy;
        if (distSq < kPickRadiusSq && distSq < bestDist) {
            bestDist = distSq;
            best = i;
        }
    }
    return best;
}

bool Viewport3D::pickSocket(const QPoint& screenPos, int& outIntersectionIdx, int& outSocketIdx) const {
    const float kPickRadiusSq = 16.0f * 16.0f;
    float bestDist = std::numeric_limits<float>::max();
    outIntersectionIdx = -1;
    outSocketIdx = -1;

    glm::mat4 vp = m_camera.projMatrix(m_aspect) * m_camera.viewMatrix();
    glm::vec2 mp = {float(screenPos.x()), float(screenPos.y())};

    for (int ii = 0; ii < (int)m_network.intersections.size(); ++ii) {
        const auto& ix = m_network.intersections[ii];
        for (int si = 0; si < (int)ix.sockets.size(); ++si) {
            const auto& socket = ix.sockets[si];
            if (!socket.enabled) continue;

            glm::vec3 worldPos = ix.pos + socket.localPos;
            glm::vec4 clip = vp * glm::vec4(-worldPos.x, worldPos.y, worldPos.z, 1.0f);
            if (clip.w <= 0.0f) continue;
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            float sx = (ndc.x + 1.0f) * 0.5f * width();
            float sy = (1.0f - ndc.y) * 0.5f * height();
            float dx = sx - mp.x;
            float dy = sy - mp.y;
            float distSq = dx * dx + dy * dy;
            if (distSq < kPickRadiusSq && distSq < bestDist) {
                bestDist = distSq;
                outIntersectionIdx = ii;
                outSocketIdx = si;
            }
        }
    }

    return outIntersectionIdx >= 0;
}

void Viewport3D::setupIxDragEndpoints(int ixIdx) {
    m_ixDragEndpoints.clear();
    if (ixIdx < 0 || ixIdx >= (int)m_network.intersections.size()) return;
    const auto& ix = m_network.intersections[ixIdx];
    for (int ri = 0; ri < (int)m_network.roads.size(); ++ri) {
        const auto& road = m_network.roads[ri];
        if (road.active == 0 || road.points.empty()) continue;
        if (road.startIntersectionId == ix.id)
            m_ixDragEndpoints.push_back({ri, 0, road.points.front().pos - ix.pos});
        if (road.endIntersectionId == ix.id)
            m_ixDragEndpoints.push_back(
                {ri, (int)road.points.size() - 1, road.points.back().pos - ix.pos});
    }
}

std::vector<SelectedPoint> Viewport3D::pickControlPointsInRect(const QRect& rect) const {
    std::vector<SelectedPoint> result;
    if (rect.isNull()) return result;

    glm::mat4 vp = m_camera.projMatrix(m_aspect) * m_camera.viewMatrix();
    QRect normalized = rect.normalized();

    for (int ri = 0; ri < static_cast<int>(m_network.roads.size()); ++ri) {
        const auto& road = m_network.roads[ri];
        for (int pi = 0; pi < static_cast<int>(road.points.size()); ++pi) {
            glm::vec3 glPos = {-road.points[pi].pos.x, road.points[pi].pos.y, road.points[pi].pos.z};
            glm::vec4 clip = vp * glm::vec4(glPos, 1.0f);
            if (clip.w <= 0.0f) continue;
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            QPoint screenPoint(
                static_cast<int>((ndc.x + 1.0f) * 0.5f * width()),
                static_cast<int>((1.0f - ndc.y) * 0.5f * height()));
            if (normalized.contains(screenPoint))
                result.push_back({ri, pi});
        }
    }

    return result;
}

bool Viewport3D::endpointHasIntersectionLink(int roadIdx, int pointIdx) const {
    if (roadIdx < 0 || roadIdx >= static_cast<int>(m_network.roads.size()))
        return false;

    const auto& road = m_network.roads[roadIdx];
    if (pointIdx == 0)
        return !road.startIntersectionId.empty() || road.startLink.connected();
    if (pointIdx == static_cast<int>(road.points.size()) - 1)
        return !road.endIntersectionId.empty() || road.endLink.connected();
    return false;
}

bool Viewport3D::canConnectEndpointToSelectedSocket(int roadIdx, int pointIdx) const {
    if (!m_editor.sel.hasSocket()) return false;
    if (roadIdx < 0 || roadIdx >= static_cast<int>(m_network.roads.size())) return false;
    const auto& road = m_network.roads[roadIdx];
    return pointIdx == 0 || pointIdx == static_cast<int>(road.points.size()) - 1;
}

void Viewport3D::syncLinkedEndpointsForIntersection(int intersectionIdx) {
    if (intersectionIdx < 0 || intersectionIdx >= static_cast<int>(m_network.intersections.size()))
        return;

    const auto& ix = m_network.intersections[intersectionIdx];
    for (auto& road : m_network.roads) {
        if (!road.points.empty() &&
            road.startLink.intersectionId == ix.id &&
            !road.startLink.socketId.empty()) {
            if (const auto* socket = m_network.findSocket(ix.id, road.startLink.socketId))
                road.points.front().pos = ix.pos + socket->localPos;
        }
        if (!road.points.empty() &&
            road.endLink.intersectionId == ix.id &&
            !road.endLink.socketId.empty()) {
            if (const auto* socket = m_network.findSocket(ix.id, road.endLink.socketId))
                road.points.back().pos = ix.pos + socket->localPos;
        }
    }
}

void Viewport3D::connectEndpointToSelectedSocket(int roadIdx, int pointIdx) {
    if (!canConnectEndpointToSelectedSocket(roadIdx, pointIdx)) return;
    if (m_editor.sel.intersectionIdx < 0 ||
        m_editor.sel.intersectionIdx >= static_cast<int>(m_network.intersections.size()))
        return;

    auto& road = m_network.roads[roadIdx];
    const auto& ix = m_network.intersections[m_editor.sel.intersectionIdx];
    if (m_editor.sel.socketIdx < 0 || m_editor.sel.socketIdx >= static_cast<int>(ix.sockets.size()))
        return;
    const auto& socket = ix.sockets[m_editor.sel.socketIdx];

    m_editor.pushUndo(m_network);
    if (pointIdx == 0) {
        road.startIntersectionId = ix.id;
        road.startLink.intersectionId = ix.id;
        road.startLink.socketId = socket.id;
        road.points.front().pos = ix.pos + socket.localPos;
    } else {
        road.endIntersectionId = ix.id;
        road.endLink.intersectionId = ix.id;
        road.endLink.socketId = socket.id;
        road.points.back().pos = ix.pos + socket.localPos;
    }
    m_editor.sel.setSinglePoint(roadIdx, pointIdx);
    makeCurrent();
    m_roadRenderer.rebuild(this, m_network);
    syncSelectionVisuals();
    doneCurrent();
    emit networkChanged();
}

void Viewport3D::detachEndpointFromIntersection(int roadIdx, int pointIdx) {
    if (roadIdx < 0 || roadIdx >= static_cast<int>(m_network.roads.size()))
        return;

    auto& road = m_network.roads[roadIdx];
    bool canDetach = false;
    if (pointIdx == 0) {
        canDetach = !road.startIntersectionId.empty();
    } else if (pointIdx == static_cast<int>(road.points.size()) - 1) {
        canDetach = !road.endIntersectionId.empty();
    } else {
        return;
    }
    if (!canDetach) return;

    m_editor.pushUndo(m_network);
    if (pointIdx == 0) {
        road.startIntersectionId.clear();
        road.startLink.clear();
    } else {
        road.endIntersectionId.clear();
        road.endLink.clear();
    }
    m_editor.sel.setSinglePoint(roadIdx, pointIdx);
    makeCurrent();
    m_roadRenderer.rebuild(this, m_network);
    syncSelectionVisuals();
    doneCurrent();
    emit networkChanged();
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
            glm::vec3 glPos = {-road.points[pi].pos.x, road.points[pi].pos.y, road.points[pi].pos.z};
            glm::vec4 clip = vp * glm::vec4(glPos, 1.0f);
            if (clip.w <= 0.0f) continue;

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

void Viewport3D::mousePressEvent(QMouseEvent* e) {
    m_lastPos = e->pos();
    if (e->button() == Qt::LeftButton) {
        m_leftButtonDown = true;
        m_leftPressPos = e->pos();
    }

    if (e->button() == Qt::RightButton && m_glReady && m_editor.mode == ToolMode::Edit) {
        int ri = -1;
        int pi = -1;
        glm::vec3 rayDir = screenToRay(e->pos());
        glm::vec3 rayOri = m_camera.position();
        if (pickControlPoint(rayOri, rayDir, ri, pi)) {
            m_editor.sel.setSinglePoint(ri, pi);
            makeCurrent();
            syncSelectionVisuals();
            doneCurrent();

            QMenu menu(this);
            QAction* connectAction = nullptr;
            QAction* detachAction = nullptr;
            if (canConnectEndpointToSelectedSocket(ri, pi))
                connectAction = menu.addAction(QStringLiteral("Connect To Selected Socket"));
            if (endpointHasIntersectionLink(ri, pi))
                detachAction = menu.addAction(QStringLiteral("Detach From Intersection"));
            QAction* chosen = menu.exec(e->globalPosition().toPoint());
            if (chosen == connectAction)
                connectEndpointToSelectedSocket(ri, pi);
            else if (chosen == detachAction)
                detachEndpointFromIntersection(ri, pi);
            return;
        }
    }

    if (e->modifiers() & Qt::AltModifier) {
        if (e->button() == Qt::LeftButton) {
            m_rotating = true;
        } else if (e->button() == Qt::MiddleButton) {
            m_panning = true;
        }
        return;
    }

    if (e->button() != Qt::LeftButton || !m_glReady) return;

    appendInputDebugLog(QString("mousePress left pos=(%1,%2) mode=%3 shift=%4 alt=%5")
        .arg(e->pos().x()).arg(e->pos().y())
        .arg(m_editor.mode == ToolMode::Edit ? "Edit" : "Select")
        .arg(isShiftHeld(e->modifiers()) ? "1" : "0")
        .arg((e->modifiers() & Qt::AltModifier) ? "1" : "0"));

    if (isShiftHeld(e->modifiers())) {
        m_boxSelectPending = true;
        m_boxSelecting = false;
        m_boxSelectOrigin = e->pos();
        m_boxSelectCurrent = e->pos();
        m_boxSelectRoadCandidate = -1;
        m_dragging = false;
        m_gizmoDragAxis = TransformGizmo::Axis::None;
        m_boxRubberBand->hide();
        setCursor(Qt::CrossCursor);
        appendInputDebugLog("boxSelect forced start by Shift");
        update();
        return;
    }

    glm::vec3 rayDir = screenToRay(e->pos());
    glm::vec3 rayOri = m_camera.position();
    glm::vec3 rayDirGl = toGlVector(rayDir);
    glm::vec3 rayOriGl = cameraGlPos();

    if (m_editor.mode == ToolMode::Select) {
        int ri = pickRoad(e->pos());
        if (ri >= 0) {
            m_editor.sel.clear();
            m_editor.sel.roadIdx = ri;
        } else {
            m_editor.sel.clear();
        }
        makeCurrent();
        syncSelectionVisuals();
        doneCurrent();
        m_dragging = false;
        return;
    }

    if (m_editor.sel.valid() || m_editor.sel.hasIntersection()) {
        glm::vec3 glPos = selectionPivotGlPos();
        glm::mat4 vpMat = m_camera.projMatrix(m_aspect) * m_camera.viewMatrix();
        QPoint pivotScreen;
        bool pivotScreenValid = false;
        glm::vec4 clip = vpMat * glm::vec4(glPos, 1.0f);
        if (clip.w > 0.0f) {
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            pivotScreen = QPoint(
                static_cast<int>((ndc.x + 1.0f) * 0.5f * width()),
                static_cast<int>((1.0f - ndc.y) * 0.5f * height()));
            pivotScreenValid = true;
        }
        auto axis = m_transformGizmo.hitTest(
            e->pos(), glPos, cameraGlPos(), vpMat, width(), height());
        if (axis == TransformGizmo::Axis::None && pivotScreenValid) {
            QPoint delta = e->pos() - pivotScreen;
            int distSq = delta.x() * delta.x() + delta.y() * delta.y();
            if (distSq <= kScreenHandlePriorityRadiusPx * kScreenHandlePriorityRadiusPx) {
                axis = TransformGizmo::Axis::Screen;
                appendInputDebugLog("gizmo center-bias fallback -> Screen");
            }
        }
        appendInputDebugLog(QString("gizmo hit axis=%1 pivotGl=(%2,%3,%4)")
            .arg(axisName(axis))
            .arg(glPos.x, 0, 'f', 3)
            .arg(glPos.y, 0, 'f', 3)
            .arg(glPos.z, 0, 'f', 3));

        if (axis != TransformGizmo::Axis::None) {
            m_gizmoDragAxis      = axis;
            m_gizmoDragOrigGlPos = glPos;
            m_gizmoDragLastGlPos = glPos;
            m_skipNextScreenDragMove = (axis == TransformGizmo::Axis::Screen);

            if (axis == TransformGizmo::Axis::Screen) {
                glm::mat4 vpMat = m_camera.projMatrix(m_aspect) * m_camera.viewMatrix();
                glm::vec4 clip = vpMat * glm::vec4(glPos, 1.0f);
                m_gizmoDragScreenDepth = (clip.w != 0.0f) ? (clip.z / clip.w) : 0.0f;
                m_gizmoDragLastGlPos = screenToGlAtDepth(e->pos(), m_gizmoDragScreenDepth);
            } else {
                m_gizmoDragT0 = TransformGizmo::axisTParam(
                    rayOriGl, rayDirGl, glPos, TransformGizmo::axisDir(axis));
            }

            if (m_editor.sel.valid())
                beginPointDrag(glPos);
            else if (m_editor.sel.hasSocket()) {
                m_socketDragIntersectionIdx = m_editor.sel.intersectionIdx;
                m_socketDragSocketIdx = m_editor.sel.socketIdx;
                if (m_socketDragIntersectionIdx >= 0 &&
                    m_socketDragIntersectionIdx < (int)m_network.intersections.size() &&
                    m_socketDragSocketIdx >= 0 &&
                    m_socketDragSocketIdx < (int)m_network.intersections[m_socketDragIntersectionIdx].sockets.size()) {
                    m_socketDragOrigLocalPos =
                        m_network.intersections[m_socketDragIntersectionIdx].sockets[m_socketDragSocketIdx].localPos;
                    m_dragging = true;
                    m_editor.pushUndo(m_network);
                }
            }
            else if (m_editor.sel.hasIntersection()) {
                setupIxDragEndpoints(m_editor.sel.intersectionIdx);
                m_dragging = true;
                m_editor.pushUndo(m_network);
            }
            return;
        }
    }

    int socketIntersectionIdx = -1;
    int socketIdx = -1;
    if (pickSocket(e->pos(), socketIntersectionIdx, socketIdx)) {
        m_editor.sel.setIntersection(socketIntersectionIdx, socketIdx);
        m_gizmoDragAxis = TransformGizmo::Axis::None;
        makeCurrent();
        syncSelectionVisuals();
        doneCurrent();
        return;
    }

    int ixIdx = pickIntersection(e->pos());
    if (ixIdx >= 0) {
        m_editor.sel.setIntersection(ixIdx);
        const auto& ix = m_network.intersections[ixIdx];
        m_gizmoDragAxis = TransformGizmo::Axis::None;
        m_dragPlaneY = {-ix.pos.x, ix.pos.y, ix.pos.z};
        setupIxDragEndpoints(ixIdx);
        m_dragging = true;
        makeCurrent();
        syncSelectionVisuals();
        doneCurrent();
        m_editor.pushUndo(m_network);
        return;
    }

    int ri = -1;
    int pi = -1;
    if (pickControlPoint(rayOri, rayDir, ri, pi)) {
        appendInputDebugLog(QString("point pick select-only road=%1 point=%2").arg(ri).arg(pi));
        m_editor.sel.setSinglePoint(ri, pi);
        m_gizmoDragAxis = TransformGizmo::Axis::None;
        makeCurrent();
        syncSelectionVisuals();
        doneCurrent();
        return;
    }

    m_boxSelectPending = true;
    m_boxSelecting = false;
    m_boxSelectOrigin = e->pos();
    m_boxSelectCurrent = e->pos();
    m_boxSelectRoadCandidate = pickRoad(e->pos());
    m_dragging = false;
    appendInputDebugLog(QString("boxSelect candidate start road=%1")
        .arg(m_boxSelectRoadCandidate));
}

void Viewport3D::mouseMoveEvent(QMouseEvent* e) {
    QPoint delta = e->pos() - m_lastPos;
    m_lastPos = e->pos();

    if (m_rotating)
        m_camera.orbit(static_cast<float>(delta.x()), static_cast<float>(delta.y()));
    if (m_panning)
        m_camera.pan(static_cast<float>(delta.x()), static_cast<float>(delta.y()));

    if (!m_boxSelectPending && !m_dragging && m_leftButtonDown &&
        isShiftHeld(e->modifiers())) {
        m_boxSelectPending = true;
        m_boxSelecting = false;
        m_boxSelectOrigin = m_leftPressPos;
        m_boxSelectCurrent = e->pos();
        m_boxSelectRoadCandidate = -1;
        m_gizmoDragAxis = TransformGizmo::Axis::None;
        m_boxRubberBand->hide();
        setCursor(Qt::CrossCursor);
        appendInputDebugLog(QString("boxSelect late start origin=(%1,%2) current=(%3,%4)")
            .arg(m_boxSelectOrigin.x()).arg(m_boxSelectOrigin.y())
            .arg(e->pos().x()).arg(e->pos().y()));
    }

    if (m_boxSelectPending && (e->buttons() & Qt::LeftButton)) {
        m_boxSelectCurrent = e->pos();
        if ((m_boxSelectCurrent - m_boxSelectOrigin).manhattanLength() >= kBoxSelectThresholdPx) {
            m_boxSelecting = true;
            m_boxRubberBand->setGeometry(QRect(m_boxSelectOrigin, m_boxSelectCurrent).normalized());
            m_boxRubberBand->show();
            m_boxRubberBand->raise();
            appendInputDebugLog(QString("boxSelect visible rect=(%1,%2)-(%3,%4)")
                .arg(m_boxSelectOrigin.x()).arg(m_boxSelectOrigin.y())
                .arg(m_boxSelectCurrent.x()).arg(m_boxSelectCurrent.y()));
        }
        setCursor(Qt::CrossCursor);
        update();
    }

    if (m_dragging && !(e->modifiers() & Qt::AltModifier)) {
        glm::vec3 rayDir = screenToRay(e->pos());
        glm::vec3 rayOri = m_camera.position();
        glm::vec3 rayDirGl = toGlVector(rayDir);
        glm::vec3 rayOriGl = cameraGlPos();
        auto resolveScreenGlHit = [&]() -> std::pair<glm::vec3, bool> {
            Q_UNUSED(rayOri);
            Q_UNUSED(rayDirGl);
            Q_UNUSED(rayOriGl);
            return {screenToGlAtDepth(e->pos(), m_gizmoDragScreenDepth), true};
        };

        if (m_gizmoDragAxis == TransformGizmo::Axis::Screen && m_skipNextScreenDragMove) {
            auto [glHit, ok] = resolveScreenGlHit();
            if (ok) m_gizmoDragLastGlPos = glHit;
            appendInputDebugLog("drag move type=Screen prime");
            m_skipNextScreenDragMove = false;
            return;
        }

        auto resolveGlPos = [&]() -> glm::vec3 {
            if (m_gizmoDragAxis != TransformGizmo::Axis::None) {
                glm::vec3 axDir = TransformGizmo::axisDir(m_gizmoDragAxis);
                float t = TransformGizmo::axisTParam(rayOriGl, rayDirGl, m_gizmoDragOrigGlPos, axDir);
                return m_gizmoDragOrigGlPos + (t - m_gizmoDragT0) * axDir;
            }
            return rayHitY(rayOri, rayDir, m_dragPlaneY.y);
        };

        if (m_editor.sel.valid()) {
            if (m_gizmoDragAxis == TransformGizmo::Axis::Screen) {
                appendInputDebugLog("drag move type=Screen points");
                auto [glHit, ok] = resolveScreenGlHit();
                if (!ok) return;
                glm::vec3 deltaGl = glHit - m_gizmoDragLastGlPos;
                if (glm::length(deltaGl) > 30.0f) {
                    appendInputDebugLog(QString("screenDrag jump points deltaGl=(%1,%2,%3) len=%4 mouse=(%5,%6) depth=%7")
                        .arg(deltaGl.x, 0, 'f', 3)
                        .arg(deltaGl.y, 0, 'f', 3)
                        .arg(deltaGl.z, 0, 'f', 3)
                        .arg(glm::length(deltaGl), 0, 'f', 3)
                        .arg(e->pos().x()).arg(e->pos().y())
                        .arg(m_gizmoDragScreenDepth, 0, 'f', 6));
                }
                glm::vec3 deltaWorld = {-deltaGl.x, deltaGl.y, deltaGl.z};
                for (const auto& selPt : m_editor.sel.points) {
                    auto& pos = m_network.roads[selPt.roadIdx].points[selPt.pointIdx].pos;
                    pos += deltaWorld;
                }
                m_gizmoDragLastGlPos = glHit;
            } else {
                appendInputDebugLog(QString("drag move type=%1 points").arg(axisName(m_gizmoDragAxis)));
                glm::vec3 newPivotGlPos = resolveGlPos();
                glm::vec3 deltaGl = newPivotGlPos - m_gizmoDragOrigGlPos;
                glm::vec3 deltaWorld = {
                    (m_gizmoDragAxis == TransformGizmo::Axis::X) ? deltaGl.x : -deltaGl.x,
                    deltaGl.y,
                    deltaGl.z
                };

                for (const auto& origin : m_pointDragOrigins) {
                    auto& pos = m_network.roads[origin.point.roadIdx].points[origin.point.pointIdx].pos;
                    pos = origin.pos + deltaWorld;
                }
            }

            makeCurrent();
            if (m_socketDragIntersectionIdx >= 0)
                syncLinkedEndpointsForIntersection(m_socketDragIntersectionIdx);
            m_roadRenderer.rebuild(this, m_network);
            syncSelectionVisuals();
            doneCurrent();
            emit networkChanged();
        } else if (m_editor.sel.hasSocket()) {
            if (m_socketDragIntersectionIdx < 0 ||
                m_socketDragIntersectionIdx >= (int)m_network.intersections.size() ||
                m_socketDragSocketIdx < 0 ||
                m_socketDragSocketIdx >= (int)m_network.intersections[m_socketDragIntersectionIdx].sockets.size()) {
                return;
            }

            auto& socket =
                m_network.intersections[m_socketDragIntersectionIdx].sockets[m_socketDragSocketIdx];
            if (m_gizmoDragAxis == TransformGizmo::Axis::Screen) {
                appendInputDebugLog("drag move type=Screen socket");
                auto [glHit, ok] = resolveScreenGlHit();
                if (!ok) return;
                glm::vec3 deltaGl = glHit - m_gizmoDragLastGlPos;
                glm::vec3 deltaWorld = {-deltaGl.x, deltaGl.y, deltaGl.z};
                socket.localPos += deltaWorld;
                m_gizmoDragLastGlPos = glHit;
            } else {
                appendInputDebugLog(QString("drag move type=%1 socket").arg(axisName(m_gizmoDragAxis)));
                glm::vec3 newGlPos = resolveGlPos();
                glm::vec3 deltaGl = newGlPos - m_gizmoDragOrigGlPos;
                glm::vec3 deltaWorld = {
                    (m_gizmoDragAxis == TransformGizmo::Axis::X) ? deltaGl.x : -deltaGl.x,
                    deltaGl.y,
                    deltaGl.z
                };
                socket.localPos = m_socketDragOrigLocalPos + deltaWorld;
            }

            makeCurrent();
            m_roadRenderer.rebuild(this, m_network);
            syncSelectionVisuals();
            doneCurrent();
            emit networkChanged();
        } else if (m_editor.sel.hasIntersection()) {
            int selIxIdx = m_editor.sel.intersectionIdx;
            if (selIxIdx >= 0 && selIxIdx < (int)m_network.intersections.size()) {
                if (m_gizmoDragAxis == TransformGizmo::Axis::Screen) {
                    appendInputDebugLog("drag move type=Screen intersection");
                    auto [glHit, ok] = resolveScreenGlHit();
                    if (!ok) return;
                    glm::vec3 deltaGl = glHit - m_gizmoDragLastGlPos;
                    if (glm::length(deltaGl) > 30.0f) {
                        appendInputDebugLog(QString("screenDrag jump ix deltaGl=(%1,%2,%3) len=%4 mouse=(%5,%6) depth=%7")
                            .arg(deltaGl.x, 0, 'f', 3)
                            .arg(deltaGl.y, 0, 'f', 3)
                            .arg(deltaGl.z, 0, 'f', 3)
                            .arg(glm::length(deltaGl), 0, 'f', 3)
                            .arg(e->pos().x()).arg(e->pos().y())
                            .arg(m_gizmoDragScreenDepth, 0, 'f', 6));
                    }
                    glm::vec3 deltaWorld = {-deltaGl.x, deltaGl.y, deltaGl.z};
                    m_network.intersections[selIxIdx].pos += deltaWorld;
                    for (const auto& ep : m_ixDragEndpoints)
                        m_network.roads[ep.roadIdx].points[ep.ptIdx].pos += deltaWorld;
                    m_gizmoDragLastGlPos = glHit;
                } else {
                    appendInputDebugLog(QString("drag move type=%1 intersection").arg(axisName(m_gizmoDragAxis)));
                    glm::vec3 newGlPos = resolveGlPos();
                    glm::vec3 newWorld = {
                        (m_gizmoDragAxis == TransformGizmo::Axis::X) ? newGlPos.x : -newGlPos.x,
                        newGlPos.y,
                        newGlPos.z
                    };
                    m_network.intersections[selIxIdx].pos = newWorld;
                    for (const auto& ep : m_ixDragEndpoints)
                        m_network.roads[ep.roadIdx].points[ep.ptIdx].pos = newWorld + ep.origOffset;
                }
                makeCurrent();
                m_roadRenderer.rebuild(this, m_network);
                syncSelectionVisuals();
                doneCurrent();
                emit networkChanged();
            }
        }
    }

    if (!m_dragging && !m_boxSelecting && m_glReady && m_editor.mode == ToolMode::Edit) {
        const bool hasGizmo = m_editor.sel.valid() || m_editor.sel.hasIntersection();
        if (hasGizmo) {
            glm::vec3 glPos = selectionPivotGlPos();
            glm::mat4 vpMat = m_camera.projMatrix(m_aspect) * m_camera.viewMatrix();
            auto newHover = m_transformGizmo.hitTest(
                e->pos(), glPos, cameraGlPos(), vpMat, width(), height());
            if (newHover != m_gizmoHover) {
                m_gizmoHover = newHover;
                update();
            }
        } else if (m_gizmoHover != TransformGizmo::Axis::None) {
            m_gizmoHover = TransformGizmo::Axis::None;
            update();
        }
    }
}

void Viewport3D::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_leftButtonDown = false;
        appendInputDebugLog(QString("mouseRelease left pos=(%1,%2) pending=%3 selecting=%4")
            .arg(e->pos().x()).arg(e->pos().y())
            .arg(m_boxSelectPending ? "1" : "0")
            .arg(m_boxSelecting ? "1" : "0"));
        if (m_boxSelectPending) {
            if (m_boxSelecting) {
                auto selected = pickControlPointsInRect(
                    QRect(m_boxSelectOrigin, m_boxSelectCurrent).normalized());
                m_editor.sel.setPoints(std::move(selected));
                appendInputDebugLog(QString("boxSelect result count=%1")
                    .arg(static_cast<int>(m_editor.sel.points.size())));
            } else {
                if (m_boxSelectRoadCandidate >= 0) {
                    m_editor.sel.clear();
                    m_editor.sel.roadIdx = m_boxSelectRoadCandidate;
                    appendInputDebugLog(QString("boxSelect click fallback road=%1")
                        .arg(m_boxSelectRoadCandidate));
                } else {
                    m_editor.sel.clear();
                    appendInputDebugLog("boxSelect cleared selection");
                }
            }
            makeCurrent();
            syncSelectionVisuals();
            doneCurrent();
        }

        m_boxSelectPending = false;
        m_boxSelecting = false;
        m_boxSelectRoadCandidate = -1;
        m_boxRubberBand->hide();
        unsetCursor();
        m_dragging = false;
        m_gizmoDragAxis = TransformGizmo::Axis::None;
        m_skipNextScreenDragMove = false;
        m_pointDragOrigins.clear();
        m_socketDragIntersectionIdx = -1;
        m_socketDragSocketIdx = -1;
        m_socketDragOrigLocalPos = {0, 0, 0};
    }

    m_rotating = false;
    m_panning = false;
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
    if (!m_glReady) return;

    if (e->modifiers() & Qt::ControlModifier) {
        if (e->key() == Qt::Key_Z) {
            if (m_editor.undo(m_network)) {
                makeCurrent();
                m_roadRenderer.rebuild(this, m_network);
                syncSelectionVisuals();
                doneCurrent();
            }
            return;
        }
        if (e->key() == Qt::Key_Y) {
            if (m_editor.redo(m_network)) {
                makeCurrent();
                m_roadRenderer.rebuild(this, m_network);
                syncSelectionVisuals();
                doneCurrent();
            }
            return;
        }
    }

    QOpenGLWidget::keyPressEvent(e);
}

void Viewport3D::keyReleaseEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Alt)
        m_altDown = false;
    QOpenGLWidget::keyReleaseEvent(e);
}
