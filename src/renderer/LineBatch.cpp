#include "LineBatch.h"
#include <glm/gtc/type_ptr.hpp>

LineBatch::~LineBatch() {
    // Call destroy() before this destructor when the GL context is still current.
}

void LineBatch::init(QOpenGLFunctions_4_1_Core* f) {
    f->glGenVertexArrays(1, &m_vao);
    f->glGenBuffers(1, &m_vbo);

    f->glBindVertexArray(m_vao);
    f->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                             sizeof(Vertex), (void*)offsetof(Vertex, pos));
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                             sizeof(Vertex), (void*)offsetof(Vertex, color));

    f->glBindVertexArray(0);
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_ready = true;
}

void LineBatch::begin() {
    m_verts.clear();
}

void LineBatch::addLine(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color) {
    m_verts.push_back({a, color});
    m_verts.push_back({b, color});
}

void LineBatch::upload(QOpenGLFunctions_4_1_Core* f) {
    m_count = static_cast<int>(m_verts.size());
    if (m_count == 0) return;

    f->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    f->glBufferData(GL_ARRAY_BUFFER,
                    m_count * static_cast<GLsizei>(sizeof(Vertex)),
                    m_verts.data(),
                    GL_DYNAMIC_DRAW);
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void LineBatch::draw(QOpenGLFunctions_4_1_Core* f, Shader& sh, const glm::mat4& vp) {
    if (!m_ready || m_count == 0) return;

    sh.bind();
    sh.setMat4(f, "u_vp", vp);
    f->glBindVertexArray(m_vao);
    f->glDrawArrays(GL_LINES, 0, m_count);
    f->glBindVertexArray(0);
    sh.unbind();
}

void LineBatch::destroy(QOpenGLFunctions_4_1_Core* f) {
    if (m_vao) { f->glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo) { f->glDeleteBuffers(1, &m_vbo);      m_vbo = 0; }
    m_ready = false;
    m_count = 0;
}
