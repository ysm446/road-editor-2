#pragma once

#include <QString>
#include <QOpenGLFunctions_4_1_Core>
#include <vector>
#include <glm/glm.hpp>
#include "../model/RoadNetwork.h"
#include "../renderer/Mesh.h"
#include "../renderer/Shader.h"

class TerrainRenderer {
public:
    void init(QOpenGLFunctions_4_1_Core* f);
    void destroy(QOpenGLFunctions_4_1_Core* f);

    bool loadFromSettings(QOpenGLFunctions_4_1_Core* f,
                          const TerrainSettings& settings,
                          QString* errorMessage = nullptr);
    void clear(QOpenGLFunctions_4_1_Core* f);

    void draw(QOpenGLFunctions_4_1_Core* f,
              Shader& shader,
              const glm::mat4& vp,
              bool wireframe) const;

    bool hasData() const { return m_hasData; }

private:
    float sampleHeight(float x, float z) const;
    glm::vec3 buildVertexPosition(int col, int row) const;
    void buildMesh(QOpenGLFunctions_4_1_Core* f);

    Mesh               m_mesh;
    std::vector<float> m_heights;
    TerrainSettings    m_settings;
    int                m_rawWidth = 0;
    int                m_rawHeight = 0;
    int                m_meshWidth = 0;
    int                m_meshHeight = 0;
    bool               m_ready = false;
    bool               m_hasData = false;
};
