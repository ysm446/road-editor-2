#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_4_1_Core>
#include <glm/glm.hpp>
#include <QString>

class Shader {
public:
    bool load(const QString& vertPath, const QString& fragPath,
              const QString& geomPath = QString());

    void bind();
    void unbind();

    void setMat4 (QOpenGLFunctions_4_1_Core* f, const char* name, const glm::mat4& mat);
    void setVec2 (QOpenGLFunctions_4_1_Core* f, const char* name, const glm::vec2& v);
    void setVec3 (QOpenGLFunctions_4_1_Core* f, const char* name, const glm::vec3& v);
    void setVec4 (QOpenGLFunctions_4_1_Core* f, const char* name, const glm::vec4& v);
    void setFloat(QOpenGLFunctions_4_1_Core* f, const char* name, float v);
    void setInt  (QOpenGLFunctions_4_1_Core* f, const char* name, int v);

    bool isValid() const { return m_prog.isLinked(); }
    GLuint programId() const { return m_prog.programId(); }

private:
    QOpenGLShaderProgram m_prog;
};
