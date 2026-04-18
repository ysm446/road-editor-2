#include "Viewport3D.h"
#include "../model/Serializer.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
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
    doneCurrent();
}

void Viewport3D::initializeGL() {
    initializeOpenGLFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

    QString outDir    = QCoreApplication::applicationDirPath() + "/shaders/";
    QString sourceDir = QString(SOURCE_DIR) + "/shaders/";
    QString shaderDir = QDir(outDir).exists() ? outDir : sourceDir;

    if (!m_lineShader.load(shaderDir + "line.vert", shaderDir + "line.frag"))
        qWarning() << "Line shader load failed — check shaders/ directory";
    if (!m_roadShader.load(shaderDir + "road.vert", shaderDir + "road.frag"))
        qWarning() << "Road shader load failed — check shaders/ directory";

    m_grid.init(this);
    m_axisGizmo.init(this);
    m_roadRenderer.init(this);
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
    m_roadRenderer.draw(this, m_lineShader, m_roadShader, vp);
    m_axisGizmo.draw(this, m_lineShader, view, width(), height());
}

void Viewport3D::loadNetwork(const QString& path) {
    if (!m_glReady) {
        m_pendingPath = path;
        return;
    }
    makeCurrent();
    if (Serializer::loadFromFile(path, m_network))
        m_roadRenderer.rebuild(this, m_network);
    doneCurrent();
    emit networkChanged();
    emit selectionChanged(-1);
    update();
}

// --- Ray casting helpers ---

glm::vec3 Viewport3D::screenToRay(const QPoint& p) const {
    float ndcX =  (2.0f * p.x()) / width()  - 1.0f;
    float ndcY = -(2.0f * p.y()) / height() + 1.0f;

    glm::mat4 proj = m_camera.projMatrix(m_aspect);
    glm::mat4 view = m_camera.viewMatrix();
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 near4 = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 far4  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
    glm::vec3 nearP = glm::vec3(near4) / near4.w;
    glm::vec3 farP  = glm::vec3(far4)  / far4.w;
    return glm::normalize(farP - nearP);
}

glm::vec3 Viewport3D::rayHitY(const glm::vec3& origin, const glm::vec3& dir, float y) const {
    // Intersect ray with horizontal plane at height y
    float t = (std::abs(dir.y) > 1e-6f) ? (y - origin.y) / dir.y : 0.0f;
    return origin + dir * t;
}

bool Viewport3D::pickControlPoint(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                   int& outRoadIdx, int& outPointIdx)
{
    const float kPickRadiusSq = 20.0f * 20.0f; // world-space units squared
    float bestDist = std::numeric_limits<float>::max();
    outRoadIdx = outPointIdx = -1;

    glm::mat4 proj = m_camera.projMatrix(m_aspect);
    glm::mat4 view = m_camera.viewMatrix();
    glm::mat4 vp   = proj * view;

    for (int ri = 0; ri < (int)m_network.roads.size(); ++ri) {
        const auto& road = m_network.roads[ri];
        for (int pi = 0; pi < (int)road.points.size(); ++pi) {
            // Convert world pos to GL space for comparison
            glm::vec3 glPos = { -road.points[pi].pos.x,
                                  road.points[pi].pos.y,
                                  road.points[pi].pos.z };

            // Project to screen
            glm::vec4 clip = vp * glm::vec4(glPos, 1.0f);
            if (clip.w <= 0.0f) continue;
            glm::vec3 ndc = glm::vec3(clip) / clip.w;

            float sx = (ndc.x + 1.0f) * 0.5f * width();
            float sy = (1.0f - ndc.y) * 0.5f * height();

            float dx = sx - m_lastPos.x();
            float dy = sy - m_lastPos.y();
            float distSq = dx*dx + dy*dy;

            if (distSq < kPickRadiusSq && distSq < bestDist) {
                bestDist   = distSq;
                outRoadIdx  = ri;
                outPointIdx = pi;
            }
        }
    }
    return outRoadIdx >= 0;
}

// --- Mouse events ---

void Viewport3D::mousePressEvent(QMouseEvent* e) {
    m_lastPos = e->pos();

    if (e->modifiers() & Qt::AltModifier) {
        if      (e->button() == Qt::LeftButton)   m_rotating = true;
        else if (e->button() == Qt::MiddleButton) m_panning  = true;
        return;
    }

    if (e->button() == Qt::LeftButton && m_glReady) {
        glm::vec3 rayDir = screenToRay(e->pos());
        glm::vec3 rayOri = m_camera.position();

        int ri, pi;
        if (pickControlPoint(rayOri, rayDir, ri, pi)) {
            bool changed = !m_editor.sel.valid() ||
                           m_editor.sel.roadIdx != ri || m_editor.sel.pointIdx != pi;
            if (changed) {
                m_editor.sel.roadIdx  = ri;
                m_editor.sel.pointIdx = pi;
                makeCurrent();
                m_roadRenderer.updateSelection(this, m_network, ri, pi);
                doneCurrent();
                emit selectionChanged(ri);
            }
            // Begin drag: horizontal plane at the point's Y (GL space)
            glm::vec3 glPos = { -m_network.roads[ri].points[pi].pos.x,
                                  m_network.roads[ri].points[pi].pos.y,
                                  m_network.roads[ri].points[pi].pos.z };
            m_dragging   = true;
            m_dragPlaneY = glPos;
            m_editor.pushUndo(m_network);
        } else {
            // Click on empty space: deselect
            m_editor.sel.clear();
            m_dragging = false;
            makeCurrent();
            m_roadRenderer.updateSelection(this, m_network, -1, -1);
            doneCurrent();
            emit selectionChanged(-1);
        }
    }
}

void Viewport3D::mouseMoveEvent(QMouseEvent* e) {
    QPoint delta = e->pos() - m_lastPos;
    m_lastPos    = e->pos();

    if (m_rotating) m_camera.orbit(static_cast<float>(delta.x()),
                                   static_cast<float>(delta.y()));
    if (m_panning)  m_camera.pan  (static_cast<float>(delta.x()),
                                   static_cast<float>(delta.y()));

    if (m_dragging && m_editor.sel.valid() &&
        !(e->modifiers() & Qt::AltModifier))
    {
        glm::vec3 rayDir = screenToRay(e->pos());
        glm::vec3 rayOri = m_camera.position();
        glm::vec3 hit    = rayHitY(rayOri, rayDir, m_dragPlaneY.y);

        int ri = m_editor.sel.roadIdx;
        int pi = m_editor.sel.pointIdx;
        // Convert hit (GL space) back to world space: flip X
        m_network.roads[ri].points[pi].pos = { -hit.x, hit.y, hit.z };

        makeCurrent();
        m_roadRenderer.rebuild(this, m_network);
        m_roadRenderer.updateSelection(this, m_network, ri, pi);
        doneCurrent();
        emit networkChanged();
    }
}

void Viewport3D::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) m_dragging = false;
    m_rotating = false;
    m_panning  = false;
}

void Viewport3D::wheelEvent(QWheelEvent* e) {
    if (QApplication::keyboardModifiers() & Qt::AltModifier) {
        float delta = static_cast<float>(e->angleDelta().y()) / 120.0f;
        m_camera.zoom(delta);
    }
}

void Viewport3D::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Alt) { m_altDown = true; return; }
    if (!m_glReady) return;

    if (e->modifiers() & Qt::ControlModifier) {
        if (e->key() == Qt::Key_Z) {
            if (m_editor.undo(m_network)) {
                makeCurrent();
                m_roadRenderer.rebuild(this, m_network);
                m_roadRenderer.updateSelection(this, m_network,
                    m_editor.sel.roadIdx, m_editor.sel.pointIdx);
                doneCurrent();
            }
            return;
        }
        if (e->key() == Qt::Key_Y) {
            if (m_editor.redo(m_network)) {
                makeCurrent();
                m_roadRenderer.rebuild(this, m_network);
                m_roadRenderer.updateSelection(this, m_network,
                    m_editor.sel.roadIdx, m_editor.sel.pointIdx);
                doneCurrent();
            }
            return;
        }
    }
    QOpenGLWidget::keyPressEvent(e);
}

void Viewport3D::keyReleaseEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Alt) m_altDown = false;
    QOpenGLWidget::keyReleaseEvent(e);
}
