#include "TransformGizmo.h"
#include <cmath>
#include <algorithm>
#include <limits>

// ---------------------------------------------------------------------------
// Axis geometry constants
// ---------------------------------------------------------------------------
// Arrow scale relative to camera distance
static constexpr float kGizmoScale  = 0.12f;
static constexpr float kHeadRatio   = 0.20f;  // arrowhead length / total arrow len
static constexpr float kHeadRadius  = 0.40f;  // arrowhead base radius / head len

// Colors (normal / highlighted)
static const glm::vec3 kColX    = {0.90f, 0.22f, 0.22f};
static const glm::vec3 kColXHi  = {1.00f, 0.55f, 0.55f};
static const glm::vec3 kColY    = {0.22f, 0.85f, 0.22f};
static const glm::vec3 kColYHi  = {0.55f, 1.00f, 0.55f};
static const glm::vec3 kColZ    = {0.25f, 0.50f, 0.95f};
static const glm::vec3 kColZHi  = {0.55f, 0.75f, 1.00f};

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

glm::vec3 TransformGizmo::axisDir(Axis a) {
    switch (a) {
        case Axis::X: return {-1.0f,  0.0f, 0.0f}; // world X+ → GL -X
        case Axis::Y: return { 0.0f,  1.0f, 0.0f};
        case Axis::Z: return { 0.0f,  0.0f, 1.0f};
        default:      return { 0.0f,  0.0f, 0.0f};
    }
}

float TransformGizmo::axisTParam(const glm::vec3& rayOrigin,
                                  const glm::vec3& rayDir,
                                  const glm::vec3& axisPoint,
                                  const glm::vec3& axisDir)
{
    // Closest point on infinite axis to ray (both unit-length).
    // t_axis = [(w·A) - (w·D)(D·A)] / [1 - (D·A)^2]
    // where w = axisPoint - rayOrigin
    glm::vec3 w = axisPoint - rayOrigin;
    float b     = glm::dot(rayDir, axisDir);
    float denom = 1.0f - b * b;
    if (std::abs(denom) < 1e-6f) return 0.0f; // parallel
    float d = glm::dot(rayDir, w);
    float e = glm::dot(axisDir, w);
    return (b * d - e) / denom;
}

// ---------------------------------------------------------------------------
// Per-axis perpendiculars (for arrowhead geometry)
// ---------------------------------------------------------------------------
static void axisPerps(TransformGizmo::Axis a, glm::vec3& p1, glm::vec3& p2) {
    switch (a) {
        case TransformGizmo::Axis::X:
            p1 = {0.0f, 1.0f, 0.0f};
            p2 = {0.0f, 0.0f, 1.0f};
            break;
        case TransformGizmo::Axis::Y:
            p1 = {-1.0f, 0.0f, 0.0f};
            p2 = { 0.0f, 0.0f, 1.0f};
            break;
        case TransformGizmo::Axis::Z:
            p1 = {-1.0f, 0.0f, 0.0f};
            p2 = { 0.0f, 1.0f, 0.0f};
            break;
        default:
            p1 = p2 = {0.0f, 0.0f, 0.0f};
    }
}

// Build one arrow: shaft + arrowhead, appended into LineBatch.
static void buildArrow(LineBatch& batch,
                       const glm::vec3& origin,
                       const glm::vec3& dir,
                       float len,
                       const glm::vec3& col,
                       TransformGizmo::Axis axis)
{
    glm::vec3 tip  = origin + len * dir;
    float headLen  = len * kHeadRatio;
    float r        = headLen * kHeadRadius;
    glm::vec3 base = tip - headLen * dir;

    // Shaft
    batch.addLine(origin, base, col);

    // Arrowhead — 4 lines from tip to cone base corners
    glm::vec3 p1, p2;
    axisPerps(axis, p1, p2);

    batch.addLine(tip, base + r * p1,  col);
    batch.addLine(tip, base - r * p1,  col);
    batch.addLine(tip, base + r * p2,  col);
    batch.addLine(tip, base - r * p2,  col);
    // Close the base diamond
    batch.addLine(base + r * p1, base + r * p2, col);
    batch.addLine(base + r * p2, base - r * p1, col);
    batch.addLine(base - r * p1, base - r * p2, col);
    batch.addLine(base - r * p2, base + r * p1, col);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void TransformGizmo::init(QOpenGLFunctions_4_1_Core* f) {
    m_batch.init(f);
    m_ready = true;
}

void TransformGizmo::rebuild(QOpenGLFunctions_4_1_Core* f,
                              const glm::vec3& vertexGlPos,
                              const glm::vec3& camGlPos,
                              Axis highlight)
{
    if (!m_ready) return;
    float len = glm::distance(vertexGlPos, camGlPos) * kGizmoScale;

    m_batch.begin();
    buildArrow(m_batch, vertexGlPos, axisDir(Axis::X), len,
               (highlight == Axis::X) ? kColXHi : kColX, Axis::X);
    buildArrow(m_batch, vertexGlPos, axisDir(Axis::Y), len,
               (highlight == Axis::Y) ? kColYHi : kColY, Axis::Y);
    buildArrow(m_batch, vertexGlPos, axisDir(Axis::Z), len,
               (highlight == Axis::Z) ? kColZHi : kColZ, Axis::Z);
    m_batch.upload(f);
}

void TransformGizmo::draw(QOpenGLFunctions_4_1_Core* f,
                           Shader& lineShader, const glm::mat4& vp)
{
    if (!m_ready) return;
    m_batch.draw(f, lineShader, vp);
}

TransformGizmo::Axis TransformGizmo::hitTest(
    const QPoint& screenPos,
    const glm::vec3& vertexGlPos,
    const glm::vec3& camGlPos,
    const glm::mat4& vpMatrix,
    int vpW, int vpH) const
{
    const float kPickRadiusSq = 10.0f * 10.0f;
    float bestDist = std::numeric_limits<float>::max();
    Axis  bestAxis = Axis::None;
    float len = glm::distance(vertexGlPos, camGlPos) * kGizmoScale;

    auto project = [&](const glm::vec3& p) -> std::pair<glm::vec2, bool> {
        glm::vec4 clip = vpMatrix * glm::vec4(p, 1.0f);
        if (clip.w <= 0.0f) return {{}, false};
        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        return {{ (ndc.x + 1.0f) * 0.5f * vpW,
                  (1.0f - ndc.y) * 0.5f * vpH }, true};
    };

    glm::vec2 mp = {float(screenPos.x()), float(screenPos.y())};

    for (int i = 0; i < 3; ++i) {
        Axis  a    = static_cast<Axis>(i);
        glm::vec3 tip = vertexGlPos + len * axisDir(a);

        auto [sa, okA] = project(vertexGlPos);
        auto [sb, okB] = project(tip);
        if (!okA || !okB) continue;

        glm::vec2 ab   = sb - sa;
        float     lsq  = glm::dot(ab, ab);
        float     distSq;

        if (lsq < 1e-6f) {
            glm::vec2 d = mp - sa;
            distSq = glm::dot(d, d);
        } else {
            float     t    = std::clamp(glm::dot(mp - sa, ab) / lsq, 0.0f, 1.0f);
            glm::vec2 proj = sa + t * ab;
            glm::vec2 d    = mp - proj;
            distSq = glm::dot(d, d);
        }

        if (distSq < kPickRadiusSq && distSq < bestDist) {
            bestDist = distSq;
            bestAxis = a;
        }
    }

    return bestAxis;
}

void TransformGizmo::destroy(QOpenGLFunctions_4_1_Core* f) {
    m_batch.destroy(f);
    m_ready = false;
}
