#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QTimer>
#include <QPoint>
#include <QString>
#include <glm/glm.hpp>
#include "Camera.h"
#include "Grid.h"
#include "AxisGizmo.h"
#include "../renderer/Shader.h"
#include "../model/RoadNetwork.h"
#include "../scene/RoadRenderer.h"
#include "../scene/TransformGizmo.h"
#include "../editor/EditorState.h"
#include "../app/PropertiesPanel.h"

class Viewport3D : public QOpenGLWidget, protected QOpenGLFunctions_4_1_Core {
    Q_OBJECT
public:
    explicit Viewport3D(QWidget* parent = nullptr);
    ~Viewport3D() override;

    void loadNetwork(const QString& path);

    const RoadNetwork& network() const { return m_network; }

public slots:
    void setToolMode(ToolMode m);
    void applyRoadProperties(int roadIdx, RoadProperties props);
    void setWireframe(bool on);

signals:
    void selectionChanged(int roadIdx);  // -1 = deselected
    void networkChanged();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;

private:
    // Ray and pick helpers
    glm::vec3 screenToRay(const QPoint& p) const;
    bool pickControlPoint(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                          int& outRoadIdx, int& outPointIdx);
    int  pickRoad(const QPoint& screenPos) const;
    int  pickIntersection(const QPoint& screenPos) const;
    glm::vec3 rayHitY(const glm::vec3& origin, const glm::vec3& dir, float y) const;
    void setupIxDragEndpoints(int ixIdx);

    Camera         m_camera;
    Grid           m_grid;
    AxisGizmo      m_axisGizmo;
    Shader         m_lineShader;
    Shader         m_roadShader;
    Shader         m_pointShader;
    QTimer         m_timer;
    QPoint         m_lastPos;
    bool           m_rotating  = false;
    bool           m_panning   = false;
    float          m_aspect    = 1.0f;
    bool           m_glReady   = false;

    RoadNetwork    m_network;
    RoadRenderer   m_roadRenderer;
    TransformGizmo m_transformGizmo;
    EditorState    m_editor;
    QString        m_pendingPath;

    // Free-plane drag state
    bool      m_dragging    = false;
    glm::vec3 m_dragPlaneY  = {0, 0, 0};
    bool      m_altDown     = false;

    // Gizmo drag state
    TransformGizmo::Axis m_gizmoHover           = TransformGizmo::Axis::None;
    TransformGizmo::Axis m_gizmoDragAxis        = TransformGizmo::Axis::None;
    float                m_gizmoDragT0          = 0.0f;
    glm::vec3            m_gizmoDragOrigGlPos   = {0.0f, 0.0f, 0.0f};
    glm::vec3            m_gizmoDragScreenHit0  = {0.0f, 0.0f, 0.0f};

    // Intersection drag state: per-endpoint offset from intersection center (world space)
    struct IxDragEndpoint { int roadIdx, ptIdx; glm::vec3 origOffset; };
    std::vector<IxDragEndpoint> m_ixDragEndpoints;
};
