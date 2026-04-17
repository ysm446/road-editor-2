#include "Mesh.h"

Mesh::~Mesh() {
    // Call destroy() before this destructor while the GL context is still current.
}

void Mesh::init(QOpenGLFunctions_4_1_Core* f) {
    f->glGenVertexArrays(1, &m_vao);
    f->glGenBuffers(1, &m_vbo);
    f->glGenBuffers(1, &m_ebo);

    f->glBindVertexArray(m_vao);
    f->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, pos));
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, normal));
    f->glEnableVertexAttribArray(2);
    f->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             (void*)offsetof(Vertex, uv));

    f->glBindVertexArray(0);
}

void Mesh::upload(QOpenGLFunctions_4_1_Core* f,
                  const std::vector<Vertex>&   verts,
                  const std::vector<uint32_t>& indices)
{
    m_count = static_cast<int>(indices.size());
    if (m_count == 0) return;

    f->glBindVertexArray(m_vao);

    f->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    f->glBufferData(GL_ARRAY_BUFFER,
                    static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)),
                    verts.data(), GL_STATIC_DRAW);

    f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    f->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                    static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                    indices.data(), GL_STATIC_DRAW);

    f->glBindVertexArray(0);
}

void Mesh::draw(QOpenGLFunctions_4_1_Core* f) const {
    if (m_count == 0) return;
    f->glBindVertexArray(m_vao);
    f->glDrawElements(GL_TRIANGLES, m_count, GL_UNSIGNED_INT, nullptr);
    f->glBindVertexArray(0);
}

void Mesh::destroy(QOpenGLFunctions_4_1_Core* f) {
    if (m_vao) { f->glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo) { f->glDeleteBuffers(1, &m_vbo);      m_vbo = 0; }
    if (m_ebo) { f->glDeleteBuffers(1, &m_ebo);      m_ebo = 0; }
    m_count = 0;
}
