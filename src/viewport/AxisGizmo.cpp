#include "AxisGizmo.h"
#include <glm/gtc/matrix_transform.hpp>

static constexpr int   kSize   = 80;   // gizmo viewport size (px)
static constexpr int   kMargin = 12;   // from viewport edge (px)
static constexpr float kLen    = 0.8f; // axis arm length in gizmo space

void AxisGizmo::init(QOpenGLFunctions_4_1_Core* f) {
    m_batch.init(f);

    // Static geometry: 3 axes + small tick marks, built once
    m_batch.begin();

    const glm::vec3 kX = {0.90f, 0.25f, 0.25f}; // red
    const glm::vec3 kY = {0.25f, 0.85f, 0.30f}; // green
    const glm::vec3 kZ = {0.25f, 0.50f, 0.95f}; // blue

    glm::vec3 O = {0, 0, 0};
    m_batch.addLine(O, {kLen,   0,    0  }, kX);
    m_batch.addLine(O, {0,    kLen,   0  }, kY);
    m_batch.addLine(O, {0,    0,    kLen }, kZ);

    // Negative ghost arms (dimmer)
    const float kDim = 0.25f;
    m_batch.addLine(O, {-kLen*0.4f, 0,         0        }, kX * kDim);
    m_batch.addLine(O, {0,         -kLen*0.4f, 0        }, kY * kDim);
    m_batch.addLine(O, {0,          0,        -kLen*0.4f}, kZ * kDim);

    m_batch.upload(f);
    m_ready = true;
}

void AxisGizmo::draw(QOpenGLFunctions_4_1_Core* f, Shader& lineShader,
                     const glm::mat4& viewMat, int vpW, int vpH)
{
    if (!m_ready) return;

    // Rotation-only view: strip translation column
    glm::mat4 rotView = viewMat;
    rotView[3] = glm::vec4(0, 0, 0, 1);

    // Orthographic projection fitting the gizmo arms
    const float half = 1.2f;
    glm::mat4 proj = glm::ortho(-half, half, -half, half, -10.0f, 10.0f);
    glm::mat4 vp   = proj * rotView;

    // Save state and set small viewport at bottom-left
    f->glViewport(kMargin, kMargin, kSize, kSize);
    f->glDisable(GL_DEPTH_TEST);

    m_batch.draw(f, lineShader, vp);

    f->glEnable(GL_DEPTH_TEST);
    // Restore full viewport
    f->glViewport(0, 0, vpW, vpH);
}

void AxisGizmo::destroy(QOpenGLFunctions_4_1_Core* f) {
    m_batch.destroy(f);
    m_ready = false;
}
