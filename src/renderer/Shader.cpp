#include "Shader.h"
#include <QDebug>
#include <glm/gtc/type_ptr.hpp>

bool Shader::load(const QString& vertPath, const QString& fragPath,
                  const QString& geomPath) {
    if (!m_prog.addShaderFromSourceFile(QOpenGLShader::Vertex, vertPath)) {
        qWarning() << "Vertex shader compile error:" << m_prog.log();
        return false;
    }
    if (!geomPath.isEmpty() &&
        !m_prog.addShaderFromSourceFile(QOpenGLShader::Geometry, geomPath)) {
        qWarning() << "Geometry shader compile error:" << m_prog.log();
        return false;
    }
    if (!m_prog.addShaderFromSourceFile(QOpenGLShader::Fragment, fragPath)) {
        qWarning() << "Fragment shader compile error:" << m_prog.log();
        return false;
    }
    if (!m_prog.link()) {
        qWarning() << "Shader link error:" << m_prog.log();
        return false;
    }
    return true;
}

void Shader::bind()   { m_prog.bind(); }
void Shader::unbind() { m_prog.release(); }

void Shader::setMat4(QOpenGLFunctions_4_1_Core* f, const char* name, const glm::mat4& mat) {
    int loc = m_prog.uniformLocation(name);
    if (loc >= 0)
        f->glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec2(QOpenGLFunctions_4_1_Core* f, const char* name, const glm::vec2& v) {
    int loc = m_prog.uniformLocation(name);
    if (loc >= 0)
        f->glUniform2f(loc, v.x, v.y);
}

void Shader::setVec3(QOpenGLFunctions_4_1_Core* f, const char* name, const glm::vec3& v) {
    int loc = m_prog.uniformLocation(name);
    if (loc >= 0)
        f->glUniform3f(loc, v.x, v.y, v.z);
}

void Shader::setVec4(QOpenGLFunctions_4_1_Core* f, const char* name, const glm::vec4& v) {
    int loc = m_prog.uniformLocation(name);
    if (loc >= 0)
        f->glUniform4f(loc, v.x, v.y, v.z, v.w);
}

void Shader::setFloat(QOpenGLFunctions_4_1_Core* f, const char* name, float v) {
    int loc = m_prog.uniformLocation(name);
    if (loc >= 0)
        f->glUniform1f(loc, v);
}

void Shader::setInt(QOpenGLFunctions_4_1_Core* f, const char* name, int v) {
    int loc = m_prog.uniformLocation(name);
    if (loc >= 0)
        f->glUniform1i(loc, v);
}
