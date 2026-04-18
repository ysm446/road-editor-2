#pragma once

#include <QOpenGLFunctions_4_1_Core>
#include <glm/glm.hpp>
#include "../renderer/LineBatch.h"
#include "../renderer/Shader.h"

class AxisGizmo {
public:
    void init   (QOpenGLFunctions_4_1_Core* f);
    // viewMat: camera view matrix (translation is stripped internally)
    void draw   (QOpenGLFunctions_4_1_Core* f, Shader& lineShader,
                 const glm::mat4& viewMat, int vpW, int vpH);
    void destroy(QOpenGLFunctions_4_1_Core* f);

private:
    LineBatch m_batch;
    bool      m_ready = false;
};
