#pragma once

#include <vector>
#include <QOpenGLFunctions_4_1_Core>
#include <glm/glm.hpp>

// Triangle mesh with position / normal / UV. Immutable after upload.
class Mesh {
public:
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    ~Mesh();

    void init   (QOpenGLFunctions_4_1_Core* f);
    void upload (QOpenGLFunctions_4_1_Core* f,
                 const std::vector<Vertex>&   verts,
                 const std::vector<uint32_t>& indices);
    void draw   (QOpenGLFunctions_4_1_Core* f) const;
    void destroy(QOpenGLFunctions_4_1_Core* f);

    bool hasData() const { return m_count > 0; }

private:
    GLuint m_vao   = 0;
    GLuint m_vbo   = 0;
    GLuint m_ebo   = 0;
    int    m_count = 0; // index count
};
