#pragma once

#include <QOpenGLFunctions_4_1_Core>
#include <QPoint>
#include <glm/glm.hpp>
#include "../renderer/LineBatch.h"
#include "../renderer/Shader.h"

// 3-axis translation gizmo drawn at the selected control point.
// All external positions are in GL rendering space (X already flipped from world).
class TransformGizmo {
public:
    enum class Axis { None = -1, X = 0, Y = 1, Z = 2 };

    void init   (QOpenGLFunctions_4_1_Core* f);
    void destroy(QOpenGLFunctions_4_1_Core* f);

    // Rebuild arrow geometry at GL-space vertex position.
    // highlight: axis to draw brighter (hover).
    void rebuild(QOpenGLFunctions_4_1_Core* f,
                 const glm::vec3& vertexGlPos,
                 const glm::vec3& camGlPos,
                 Axis highlight = Axis::None);

    void draw(QOpenGLFunctions_4_1_Core* f, Shader& lineShader, const glm::mat4& vp);

    // Screen-space hit test on shaft lines. Returns Axis::None if no hit.
    Axis hitTest(const QPoint& screenPos,
                 const glm::vec3& vertexGlPos,
                 const glm::vec3& camGlPos,
                 const glm::mat4& vpMatrix,
                 int vpW, int vpH) const;

    // GL-space direction vector for a given axis.
    // X maps to GL (-1,0,0) because world X+ is left-handed and GL flips it.
    static glm::vec3 axisDir(Axis a);

    // Returns t along the axis at the closest point to the given ray.
    // axisPoint + t * axisDir is the world-space drag point.
    static float axisTParam(const glm::vec3& rayOrigin,
                            const glm::vec3& rayDir,
                            const glm::vec3& axisPoint,
                            const glm::vec3& axisDir);

    bool ready() const { return m_ready; }

private:
    LineBatch m_batch;
    bool      m_ready = false;
};
