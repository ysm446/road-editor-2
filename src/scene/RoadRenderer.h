#pragma once

#include <QOpenGLFunctions_4_1_Core>
#include <glm/glm.hpp>
#include "../renderer/LineBatch.h"
#include "../renderer/Mesh.h"
#include "../renderer/Shader.h"
#include "../model/RoadNetwork.h"

class RoadRenderer {
public:
    void init   (QOpenGLFunctions_4_1_Core* f);
    void rebuild(QOpenGLFunctions_4_1_Core* f, const RoadNetwork& net);

    // Update orange highlight for selected control point (-1 to clear)
    void updateSelection(QOpenGLFunctions_4_1_Core* f,
                         const RoadNetwork& net, int roadIdx, int pointIdx);

    // lineShader: for centerlines and intersection markers
    // roadShader: for road surface mesh (Lambert)
    // pointShader: for control point dots (GL_POINTS with gl_PointSize)
    void draw   (QOpenGLFunctions_4_1_Core* f,
                 Shader& lineShader, Shader& roadShader, Shader& pointShader,
                 const glm::mat4& vp);

    void setWireframe(bool on) { m_wireframe = on; }

    void destroy(QOpenGLFunctions_4_1_Core* f);

private:
    bool m_wireframe = false;
    LineBatch m_roads;       // centerlines (unselected)
    LineBatch m_nodes;       // intersection markers
    LineBatch m_selBatch;    // selected control point highlight
    LineBatch m_selRoad;     // selected road centerline highlight
    Mesh      m_surfaceMesh; // road surface quads

    bool m_ready   = false;
    bool m_hasData = false;

    static glm::vec3 toGL(const glm::vec3& p) { return {-p.x, p.y, p.z}; }
};
