#pragma once

#include <QOpenGLFunctions_4_1_Core>
#include <glm/glm.hpp>
#include "../renderer/LineBatch.h"
#include "../renderer/Shader.h"

// XZ-plane reference grid. Static geometry — built once in init().
class Grid {
public:
    void init(QOpenGLFunctions_4_1_Core* f);
    void draw(QOpenGLFunctions_4_1_Core* f, Shader& sh, const glm::mat4& vp);
    void destroy(QOpenGLFunctions_4_1_Core* f) { m_batch.destroy(f); m_ready = false; }

private:
    LineBatch m_batch;
    bool      m_ready = false;

    static constexpr int HALF = 500;
    static constexpr int STEP = 25;
};
