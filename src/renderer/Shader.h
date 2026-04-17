#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_4_1_Core>
#include <glm/glm.hpp>
#include <QString>

class Shader {
public:
    bool load(const QString& vertPath, const QString& fragPath);

    void bind();
    void unbind();

    void setMat4(QOpenGLFunctions_4_1_Core* f, const char* name, const glm::mat4& mat);
    void setVec3(QOpenGLFunctions_4_1_Core* f, const char* name, const glm::vec3& v);
    void setVec4(QOpenGLFunctions_4_1_Core* f, const char* name, const glm::vec4& v);

    bool isValid() const { return m_prog.isLinked(); }
    GLuint programId() const { return m_prog.programId(); }

private:
    QOpenGLShaderProgram m_prog;
};
