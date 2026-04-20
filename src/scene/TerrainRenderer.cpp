#include "TerrainRenderer.h"
#include <QImage>
#include <algorithm>
#include <cmath>

namespace {
constexpr int kMaxAutoMeshCells = 256;

int clampMeshCells(int requested, int rawSize) {
    if (rawSize < 2)
        return 0;
    const int maxCells = rawSize - 1;
    if (requested > 0)
        return std::clamp(requested, 1, maxCells);
    return std::min(maxCells, kMaxAutoMeshCells);
}
}

void TerrainRenderer::init(QOpenGLFunctions_4_1_Core* f) {
    m_mesh.init(f);
    m_ready = true;
}

void TerrainRenderer::destroy(QOpenGLFunctions_4_1_Core* f) {
    m_mesh.destroy(f);
    clearTexture(f);
    m_heights.clear();
    m_ready = false;
    m_hasData = false;
    m_rawWidth = 0;
    m_rawHeight = 0;
    m_meshWidth = 0;
    m_meshHeight = 0;
}

bool TerrainRenderer::loadFromSettings(QOpenGLFunctions_4_1_Core* f,
                                       const TerrainSettings& settings,
                                       QString* errorMessage) {
    if (!m_ready)
        return false;

    const QString path = QString::fromStdString(settings.path);
    QImage image(path);
    if (image.isNull()) {
        if (errorMessage)
            *errorMessage = QString("ハイトマップを開けませんでした: %1").arg(path);
        clear(f);
        return false;
    }

    image = image.convertToFormat(QImage::Format_Grayscale8);
    if (image.width() < 2 || image.height() < 2) {
        if (errorMessage)
            *errorMessage = QString("ハイトマップの解像度が不足しています: %1").arg(path);
        clear(f);
        return false;
    }

    m_settings = settings;
    m_rawWidth = image.width();
    m_rawHeight = image.height();
    m_meshWidth = clampMeshCells(settings.meshCellsX, m_rawWidth) + 1;
    m_meshHeight = clampMeshCells(settings.meshCellsZ, m_rawHeight) + 1;

    m_heights.resize(static_cast<size_t>(m_rawWidth) * static_cast<size_t>(m_rawHeight));
    for (int y = 0; y < m_rawHeight; ++y) {
        const uchar* row = image.constScanLine(y);
        for (int x = 0; x < m_rawWidth; ++x)
            m_heights[static_cast<size_t>(y) * static_cast<size_t>(m_rawWidth) + static_cast<size_t>(x)] =
                static_cast<float>(row[x]) / 255.0f;
    }

    buildMesh(f);
    if (!settings.texturePath.empty()) {
        if (!loadTexture(f, QString::fromStdString(settings.texturePath), errorMessage)) {
            clear(f);
            return false;
        }
    } else {
        clearTexture(f);
    }
    m_hasData = true;
    if (errorMessage)
        errorMessage->clear();
    return true;
}

void TerrainRenderer::clear(QOpenGLFunctions_4_1_Core* f) {
    m_mesh.upload(f, {}, {});
    clearTexture(f);
    m_heights.clear();
    m_hasData = false;
    m_rawWidth = 0;
    m_rawHeight = 0;
    m_meshWidth = 0;
    m_meshHeight = 0;
}

bool TerrainRenderer::loadTexture(QOpenGLFunctions_4_1_Core* f, const QString& path, QString* errorMessage) {
    QImage image(path);
    if (image.isNull()) {
        if (errorMessage)
            *errorMessage = QString("地形テクスチャを開けませんでした: %1").arg(path);
        return false;
    }

    image = image.convertToFormat(QImage::Format_RGBA8888).flipped(Qt::Vertical);
    if (m_texture == 0)
        f->glGenTextures(1, &m_texture);

    f->glBindTexture(GL_TEXTURE_2D, m_texture);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image.width(), image.height(),
                    0, GL_RGBA, GL_UNSIGNED_BYTE, image.constBits());
    f->glGenerateMipmap(GL_TEXTURE_2D);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    m_hasTexture = true;
    if (errorMessage)
        errorMessage->clear();
    return true;
}

void TerrainRenderer::clearTexture(QOpenGLFunctions_4_1_Core* f) {
    if (m_texture != 0) {
        f->glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    m_hasTexture = false;
}

float TerrainRenderer::sampleHeight(float x, float z) const {
    if (m_heights.empty() || m_rawWidth < 2 || m_rawHeight < 2)
        return 0.0f;

    const float clampedX = std::clamp(x, 0.0f, static_cast<float>(m_rawWidth - 1));
    const float clampedZ = std::clamp(z, 0.0f, static_cast<float>(m_rawHeight - 1));
    const int x0 = static_cast<int>(std::floor(clampedX));
    const int z0 = static_cast<int>(std::floor(clampedZ));
    const int x1 = std::min(x0 + 1, m_rawWidth - 1);
    const int z1 = std::min(z0 + 1, m_rawHeight - 1);
    const float tx = clampedX - static_cast<float>(x0);
    const float tz = clampedZ - static_cast<float>(z0);

    auto rawAt = [&](int px, int pz) {
        return m_heights[static_cast<size_t>(pz) * static_cast<size_t>(m_rawWidth) + static_cast<size_t>(px)];
    };

    const float h00 = rawAt(x0, z0);
    const float h10 = rawAt(x1, z0);
    const float h01 = rawAt(x0, z1);
    const float h11 = rawAt(x1, z1);
    const float hx0 = h00 + (h10 - h00) * tx;
    const float hx1 = h01 + (h11 - h01) * tx;
    return hx0 + (hx1 - hx0) * tz;
}

glm::vec3 TerrainRenderer::buildVertexPosition(int col, int row) const {
    const float u = (m_meshWidth > 1) ? static_cast<float>(col) / static_cast<float>(m_meshWidth - 1) : 0.0f;
    const float v = (m_meshHeight > 1) ? static_cast<float>(row) / static_cast<float>(m_meshHeight - 1) : 0.0f;

    const float rawX = u * static_cast<float>(m_rawWidth - 1);
    const float rawZ = v * static_cast<float>(m_rawHeight - 1);
    const float terrainX = m_settings.offset.x + u * m_settings.width - m_settings.width * 0.5f;
    const float terrainZ = m_settings.offset.z + v * m_settings.depth - m_settings.depth * 0.5f;
    const float terrainY = m_settings.offset.y + sampleHeight(rawX, rawZ) * m_settings.height;

    return {-terrainX, terrainY, terrainZ};
}

void TerrainRenderer::buildMesh(QOpenGLFunctions_4_1_Core* f) {
    std::vector<Mesh::Vertex> verts;
    std::vector<uint32_t> indices;
    verts.resize(static_cast<size_t>(m_meshWidth) * static_cast<size_t>(m_meshHeight));

    for (int row = 0; row < m_meshHeight; ++row) {
        for (int col = 0; col < m_meshWidth; ++col) {
            const size_t idx = static_cast<size_t>(row) * static_cast<size_t>(m_meshWidth) + static_cast<size_t>(col);
            verts[idx].pos = buildVertexPosition(col, row);
            verts[idx].uv = {
                (m_meshWidth > 1) ? static_cast<float>(col) / static_cast<float>(m_meshWidth - 1) : 0.0f,
                (m_meshHeight > 1) ? static_cast<float>(row) / static_cast<float>(m_meshHeight - 1) : 0.0f
            };
        }
    }

    for (int row = 0; row < m_meshHeight; ++row) {
        for (int col = 0; col < m_meshWidth; ++col) {
            const int left = std::max(col - 1, 0);
            const int right = std::min(col + 1, m_meshWidth - 1);
            const int down = std::max(row - 1, 0);
            const int up = std::min(row + 1, m_meshHeight - 1);

            const glm::vec3 dx = buildVertexPosition(right, row) - buildVertexPosition(left, row);
            const glm::vec3 dz = buildVertexPosition(col, up) - buildVertexPosition(col, down);
            glm::vec3 normal = glm::cross(dx, dz);
            if (glm::length(normal) < 1e-6f)
                normal = {0.0f, 1.0f, 0.0f};
            else
                normal = glm::normalize(normal);

            const size_t idx = static_cast<size_t>(row) * static_cast<size_t>(m_meshWidth) + static_cast<size_t>(col);
            verts[idx].normal = normal;
        }
    }

    indices.reserve(static_cast<size_t>(m_meshWidth - 1) * static_cast<size_t>(m_meshHeight - 1) * 6);
    for (int row = 0; row + 1 < m_meshHeight; ++row) {
        for (int col = 0; col + 1 < m_meshWidth; ++col) {
            const uint32_t i0 = static_cast<uint32_t>(row * m_meshWidth + col);
            const uint32_t i1 = static_cast<uint32_t>(row * m_meshWidth + (col + 1));
            const uint32_t i2 = static_cast<uint32_t>((row + 1) * m_meshWidth + col);
            const uint32_t i3 = static_cast<uint32_t>((row + 1) * m_meshWidth + (col + 1));
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    m_mesh.upload(f, verts, indices);
}

void TerrainRenderer::draw(QOpenGLFunctions_4_1_Core* f,
                           Shader& shader,
                           const glm::mat4& vp,
                           bool wireframe) const {
    if (!m_hasData)
        return;

    f->glDisable(GL_CULL_FACE);
    f->glEnable(GL_POLYGON_OFFSET_FILL);
    f->glPolygonOffset(6.0f, 6.0f);

    shader.bind();
    shader.setMat4(f, "u_mvp", vp);
    shader.setVec3(f, "u_sunDir", glm::normalize(glm::vec3(0.4f, 1.0f, 0.5f)));
    shader.setVec3(f, "u_color", glm::vec3(0.34f, 0.40f, 0.31f));
    shader.setInt(f, "u_texture", 0);
    shader.setInt(f, "u_useTexture", m_hasTexture ? 1 : 0);
    if (m_hasTexture) {
        f->glActiveTexture(GL_TEXTURE0);
        f->glBindTexture(GL_TEXTURE_2D, m_texture);
    }
    m_mesh.draw(f);
    if (m_hasTexture)
        f->glBindTexture(GL_TEXTURE_2D, 0);
    shader.unbind();

    f->glDisable(GL_POLYGON_OFFSET_FILL);

    if (wireframe) {
        f->glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        f->glEnable(GL_POLYGON_OFFSET_LINE);
        f->glPolygonOffset(-2.0f, -2.0f);
        shader.bind();
        shader.setMat4(f, "u_mvp", vp);
        shader.setVec3(f, "u_sunDir", glm::normalize(glm::vec3(0.4f, 1.0f, 0.5f)));
        shader.setVec3(f, "u_color", glm::vec3(0.20f, 0.26f, 0.18f));
        shader.setInt(f, "u_texture", 0);
        shader.setInt(f, "u_useTexture", 0);
        m_mesh.draw(f);
        shader.unbind();
        f->glDisable(GL_POLYGON_OFFSET_LINE);
        f->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}
