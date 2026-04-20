#pragma once

#include <vector>
#include <QOpenGLFunctions_4_1_Core>
#include <glm/glm.hpp>
#include "Shader.h"

// Dynamic line renderer. Usage: begin() → addLine()* → upload() → draw().
// For static geometry (e.g. grid) call begin/upload once, then draw every frame.
class LineBatch {
public:
    ~LineBatch();

    void init(QOpenGLFunctions_4_1_Core* f);
    void begin();
    void addLine(const glm::vec3& a, const glm::vec3& b,
                 const glm::vec3& color = {1.0f, 1.0f, 1.0f});
    void addPoint(const glm::vec3& p,
                  const glm::vec3& color = {1.0f, 1.0f, 1.0f});
    void upload(QOpenGLFunctions_4_1_Core* f);
    void draw(QOpenGLFunctions_4_1_Core* f, Shader& sh, const glm::mat4& vp);
    // Draw as GL_POINTS with the given screen-space point size (px).
    // u_pointSize uniform must exist in the shader.
    void drawPoints(QOpenGLFunctions_4_1_Core* f, Shader& sh,
                    const glm::mat4& vp, float pointSize);
    void drawScreenLines(QOpenGLFunctions_4_1_Core* f, Shader& sh, const glm::mat4& vp,
                         const glm::vec2& viewportSize, float lineWidth, float depthBias = 1e-4f);
    void destroy(QOpenGLFunctions_4_1_Core* f);

    int vertexCount() const { return m_count; }

private:
    struct Vertex { glm::vec3 pos; glm::vec3 color; };

    std::vector<Vertex> m_verts;
    GLuint m_vao   = 0;
    GLuint m_vbo   = 0;
    int    m_count = 0;
    bool   m_ready = false;
};
