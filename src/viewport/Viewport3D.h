#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_1_Core>
#include <QTimer>
#include <QPoint>
#include <QRect>
#include <QRubberBand>
#include <QString>
#include <vector>
#include <glm/glm.hpp>
#include "Camera.h"
#include "Grid.h"
#include "AxisGizmo.h"
#include "../renderer/LineBatch.h"
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
    void setEditSubTool(EditSubTool subTool);
    void applyRoadProperties(int roadIdx, RoadProperties props);
    void applySelectedVerticalCurveProperties(int roadIdx, int curveIdx, float u, float vcl, float offset);
    void removeSelectedVerticalCurve(int roadIdx, int curveIdx);
    void applySelectedBankAngleProperties(int roadIdx, int curveIdx, float u, float targetSpeed,
                                          bool useAngle, float angle);
    void removeSelectedBankAngle(int roadIdx, int curveIdx);
    void applySelectedLaneSectionProperties(
        int roadIdx, int curveIdx, float u,
        bool useLaneLeft2, float widthLaneLeft2,
        bool useLaneLeft1, float widthLaneLeft1,
        bool useLaneCenter, float widthLaneCenter,
        bool useLaneRight1, float widthLaneRight1,
        bool useLaneRight2, float widthLaneRight2,
        float offsetCenter);
    void removeSelectedLaneSection(int roadIdx, int curveIdx);
    void setWireframe(bool on);
    void applySelectedSocketProperties(const QString& name, float yaw, bool enabled);
    void addSocketToSelectedIntersection();
    void removeSelectedSocket();

signals:
    void selectionChanged(int roadIdx);  // -1 = deselected
    void selectionStateChanged(const Selection& sel);
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
    glm::vec3 screenToGlAtDepth(const QPoint& p, float ndcZ) const;
    bool pickControlPoint(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                          int& outRoadIdx, int& outPointIdx);
    bool pickControlEdge(const QPoint& screenPos, int& outRoadIdx, int& outInsertPointIdx,
                         glm::vec3& outWorldPos) const;
    bool pickVerticalCurvePoint(const QPoint& screenPos, int& outRoadIdx, int& outCurveIdx) const;
    bool pickBankAnglePoint(const QPoint& screenPos, int& outRoadIdx, int& outCurveIdx) const;
    bool pickLaneSectionPoint(const QPoint& screenPos, int& outRoadIdx, int& outCurveIdx) const;
    bool findNearestRoadU(const QPoint& screenPos, int roadIdx, float& outU) const;
    glm::vec3 sampleRoadPosition(const Road& road, float u) const;
    bool pickEndpointControlPoint(const QPoint& screenPos, int& outRoadIdx, int& outPointIdx,
                                  float pickRadiusPx) const;
    bool pickSocket(const QPoint& screenPos, int& outIntersectionIdx, int& outSocketIdx) const;
    bool pickSocket(const QPoint& screenPos, int& outIntersectionIdx, int& outSocketIdx,
                    float pickRadiusPx) const;
    int  pickRoad(const QPoint& screenPos) const;
    int  pickIntersection(const QPoint& screenPos) const;
    glm::vec3 rayHitY(const glm::vec3& origin, const glm::vec3& dir, float y) const;
    void setupIxDragEndpoints(int ixIdx);
    glm::vec3 cameraForwardWorld() const;
    glm::vec3 cameraForwardGl() const;
    glm::vec3 cameraGlPos() const;
    glm::vec3 toGlVector(const glm::vec3& v) const;
    glm::vec3 selectionPivotGlPos() const;
    std::vector<SelectedPoint> pickControlPointsInRect(const QRect& rect) const;
    bool endpointHasIntersectionLink(int roadIdx, int pointIdx) const;
    void detachEndpointFromIntersection(int roadIdx, int pointIdx);
    bool isEndpointControlPoint(int roadIdx, int pointIdx) const;
    bool canConnectEndpointToSelectedSocket(int roadIdx, int pointIdx) const;
    void connectEndpointToSocket(int roadIdx, int pointIdx, int intersectionIdx, int socketIdx);
    void connectEndpointToSelectedSocket(int roadIdx, int pointIdx);
    void syncLinkedEndpointsForIntersection(int intersectionIdx);
    void deleteSelectedControlPoints();
    void syncSelectionVisuals();
    void beginPointDrag(const glm::vec3& pivotGlPos);
    int selectedRoadForPanels() const;
    void clearCreateToolState();

    Camera         m_camera;
    Grid           m_grid;
    AxisGizmo      m_axisGizmo;
    LineBatch      m_boxOverlay;
    Shader         m_lineShader;
    Shader         m_roadShader;
    Shader         m_pointShader;
    QTimer         m_timer;
    QPoint         m_lastPos;
    bool           m_rotating  = false;
    bool           m_panning   = false;
    bool           m_leftButtonDown = false;
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
    struct PointDragOrigin { SelectedPoint point; glm::vec3 pos; };
    std::vector<PointDragOrigin> m_pointDragOrigins;
    int       m_socketDragIntersectionIdx = -1;
    int       m_socketDragSocketIdx = -1;
    glm::vec3 m_socketDragOrigLocalPos = {0, 0, 0};
    int       m_socketHoverIntersectionIdx = -1;
    int       m_socketHoverSocketIdx = -1;
    int       m_endpointHoverRoadIdx = -1;
    int       m_endpointHoverPointIdx = -1;
    bool      m_verticalCurveDragging = false;
    int       m_verticalCurveDragRoad = -1;
    int       m_verticalCurveDragPoint = -1;
    bool      m_bankAngleDragging = false;
    int       m_bankAngleDragRoad = -1;
    int       m_bankAngleDragPoint = -1;
    bool      m_laneSectionDragging = false;
    int       m_laneSectionDragRoad = -1;
    int       m_laneSectionDragPoint = -1;
    glm::vec3 m_createPreviewPos = {0.0f, 0.0f, 0.0f};
    int       m_createHoverIntersectionIdx = -1;
    int       m_createHoverSocketIdx = -1;
    int       m_pendingRoadStartIntersectionIdx = -1;
    int       m_pendingRoadStartSocketIdx = -1;
    std::vector<glm::vec3> m_pendingRoadPoints;

    // Box selection state
    bool      m_boxSelectPending = false;
    bool      m_boxSelecting     = false;
    QPoint    m_leftPressPos;
    QPoint    m_boxSelectOrigin;
    QPoint    m_boxSelectCurrent;
    int       m_boxSelectRoadCandidate = -1;
    QRubberBand* m_boxRubberBand = nullptr;

    // Gizmo drag state
    TransformGizmo::Axis m_gizmoHover           = TransformGizmo::Axis::None;
    TransformGizmo::Axis m_gizmoDragAxis        = TransformGizmo::Axis::None;
    bool                 m_skipNextScreenDragMove = false;
    float                m_gizmoDragT0          = 0.0f;
    float                m_gizmoDragScreenDepth = 0.0f;
    glm::vec3            m_gizmoDragOrigGlPos   = {0.0f, 0.0f, 0.0f};
    glm::vec3            m_gizmoDragLastGlPos   = {0.0f, 0.0f, 0.0f};
    glm::vec3            m_gizmoDragScreenHit0  = {0.0f, 0.0f, 0.0f};
    glm::vec3            m_gizmoDragOrigWorldPos = {0.0f, 0.0f, 0.0f};
    glm::vec3            m_gizmoDragLastWorldHit = {0.0f, 0.0f, 0.0f};

    // Intersection drag state: per-endpoint offset from intersection center (world space)
    struct IxDragEndpoint { int roadIdx, ptIdx; glm::vec3 origOffset; };
    std::vector<IxDragEndpoint> m_ixDragEndpoints;
};
