#include "Grid.h"

void Grid::init(QOpenGLFunctions_4_1_Core* f) {
    m_batch.init(f);
    m_batch.begin();

    const glm::vec3 kGrid  = {0.22f, 0.22f, 0.22f};
    const glm::vec3 kXAxis = {0.75f, 0.20f, 0.20f}; // red  — X axis line
    const glm::vec3 kZAxis = {0.20f, 0.20f, 0.75f}; // blue — Z axis line

    // Lines parallel to Z (varying X)
    for (int x = -HALF; x <= HALF; x += STEP) {
        glm::vec3 c = (x == 0) ? kXAxis : kGrid;
        m_batch.addLine({(float)x, 0.0f, -(float)HALF},
                        {(float)x, 0.0f,  (float)HALF}, c);
    }
    // Lines parallel to X (varying Z)
    for (int z = -HALF; z <= HALF; z += STEP) {
        glm::vec3 c = (z == 0) ? kZAxis : kGrid;
        m_batch.addLine({-(float)HALF, 0.0f, (float)z},
                        { (float)HALF, 0.0f, (float)z}, c);
    }

    m_batch.upload(f);
    m_ready = true;
}

void Grid::draw(QOpenGLFunctions_4_1_Core* f, Shader& sh, const glm::mat4& vp) {
    if (!m_ready) return;
    m_batch.draw(f, sh, vp);
}
