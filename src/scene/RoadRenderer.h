#pragma once

#include <QOpenGLFunctions_4_1_Core>
#include <glm/glm.hpp>
#include "../renderer/LineBatch.h"
#include "../renderer/Mesh.h"
#include "../renderer/Shader.h"
#include "../model/RoadNetwork.h"
#include "../editor/EditorState.h"

class RoadRenderer {
public:
    void init   (QOpenGLFunctions_4_1_Core* f);
    void rebuild(QOpenGLFunctions_4_1_Core* f, const RoadNetwork& net);

    // Update highlight for selected control points / roads.
    void updateSelection(QOpenGLFunctions_4_1_Core* f,
                         const RoadNetwork& net, const Selection& sel);

    // lineShader: for centerlines and intersection markers
    // roadShader: for road surface mesh (Lambert)
    // pointShader: for control point dots (GL_POINTS with gl_PointSize)
    void draw   (QOpenGLFunctions_4_1_Core* f,
                 Shader& lineShader, Shader& screenLineShader,
                 Shader& roadShader, Shader& pointShader,
                 const glm::mat4& vp, const glm::vec2& viewportSize,
                 ToolMode mode);

    void setWireframe (bool on) { m_wireframe  = on; }
    void setShowPoints(bool on) { m_showPoints = on; }
    void setVerticalCurvePreviewColors(bool on) { m_verticalCurvePreviewColors = on; }
    void setBankAnglePreviewColors(bool on) { m_bankAnglePreviewColors = on; }
    void setLaneSectionPreview(bool on) { m_laneSectionPreview = on; }
    void setShowDirectionArrows(bool on) { m_showDirectionArrows = on; }

    void destroy(QOpenGLFunctions_4_1_Core* f);

private:
    bool m_wireframe   = false;
    bool m_showPoints  = true;
    bool m_verticalCurvePreviewColors = false;
    bool m_bankAnglePreviewColors = false;
    bool m_laneSectionPreview = false;
    bool m_showDirectionArrows = false;
    LineBatch m_roads;       // centerlines (unselected)
    LineBatch m_directionArrows; // direction arrows along centerlines
    LineBatch m_lanePreview; // lane boundary preview lines
    LineBatch m_nodePoints;  // intersection center points
    LineBatch m_socketLines; // socket spokes
    LineBatch m_socketPoints; // socket markers
    LineBatch m_allPoints;    // all control point dots (gray)
    LineBatch m_allCtrlLines; // control polygon for all roads (dim, Edit mode)
    LineBatch m_selBatch;     // selected control point (orange)
    LineBatch m_selRoad;      // selected road centerline highlight
    LineBatch m_selCtrlLines; // selected road control polygon highlight
    LineBatch m_selSockets;   // selected intersection sockets
    Mesh      m_surfaceMesh;       // road surface quads
    Mesh      m_intersectionMesh; // intersection fill

    bool m_ready   = false;
    bool m_hasData = false;

    static glm::vec3 toGL(const glm::vec3& p) { return p; }
};
