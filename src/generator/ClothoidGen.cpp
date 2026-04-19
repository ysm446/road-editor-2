#include "ClothoidGen.h"
#include <cmath>
#include <algorithm>
#include <cassert>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
namespace {

constexpr float kClothoidAngleRatio = 0.2f;

using v2 = glm::vec2;
using v3 = glm::vec3;

// Polynomial approximation of the normalized Fresnel X integral (cos component).
// tau = half-turning angle at the clothoid endpoint.
// A   = clothoid scale factor (A^2 = 2*tau at endpoint).
inline float clothoidX(float tau, float A) {
    float s = std::sqrt(std::max(tau, 0.0f));
    return A / std::sqrt(2.0f) * 2.0f * s *
        (1.0f - (0.1f   * std::pow(tau, 2.0f)
               + (1.0f / 216.0f) * std::pow(tau, 4.0f)
               - (1.0f / 9360.0f) * std::pow(tau, 6.0f)));
}

// Polynomial approximation of the normalized Fresnel Y integral (sin component).
inline float clothoidY(float tau, float A) {
    float s = std::sqrt(std::max(tau, 0.0f));
    return A / std::sqrt(2.0f) * (2.0f / 3.0f) * s * tau *
        (1.0f - (std::pow(tau, 2.0f) / 14.0f)
               + (std::pow(tau, 4.0f) / 440.0f)
               - (std::pow(tau, 6.0f) / 25200.0f));
}

inline v2 rotate2(v2 v, float a) {
    float c = std::cos(a), s = std::sin(a);
    return { v.x * c - v.y * s, v.x * s + v.y * c };
}

// Intersection of two lines:  p0 + t*(p1-p0)  and  p2 + s*(p3-p2).
v2 lineIntersect2D(v2 p0, v2 p1, v2 p2, v2 p3) {
    v2 d0 = p1 - p0;
    v2 d1 = p3 - p2;
    float denom = d0.x * d1.y - d0.y * d1.x;
    if (std::abs(denom) < 1e-6f) return p2; // parallel fallback
    v2 delta = p2 - p0;
    float t = (delta.x * d1.y - delta.y * d1.x) / denom;
    return p0 + t * d0;
}

void appendUnique(std::vector<v3>& out, v3 p) {
    if (!out.empty()) {
        v3 d = out.back() - p;
        if (glm::dot(d, d) < 1e-8f) return;
    }
    out.push_back(p);
}

// Local 2D coordinate frame with p0 as origin, x-axis toward p1.
struct Frame { v3 origin, xAxis, yAxis, zAxis; };

bool buildFrame(v3 p0, v3 p1, v3 p2, Frame& f) {
    v3 seg0 = p1 - p0;
    v3 seg1 = p2 - p1;
    float l0 = glm::length(seg0);
    float l1 = glm::length(seg1);
    if (l0 < 1e-4f || l1 < 1e-4f) return false;

    v3 xAxis = seg0 / l0;
    v3 zAxis = glm::cross(xAxis, seg1);
    float zLen = glm::length(zAxis);
    if (zLen < 1e-4f) return false;
    zAxis /= zLen;
    v3 yAxis = glm::normalize(glm::cross(zAxis, xAxis));
    f = { p0, xAxis, yAxis, zAxis };
    return true;
}

inline v3 worldToLocal(const Frame& f, v3 p) {
    v3 r = p - f.origin;
    return { glm::dot(r, f.xAxis), glm::dot(r, f.yAxis), glm::dot(r, f.zAxis) };
}

inline v3 localToWorld(const Frame& f, v3 p) {
    return f.origin + f.xAxis * p.x + f.yAxis * p.y + f.zAxis * p.z;
}

// Quadratic Bézier fallback when clothoid degenerates.
void appendBezier(std::vector<v3>& out, v3 p0, v3 p1, v3 p2, float interval) {
    float len = glm::length(p1 - p0) + glm::length(p2 - p1);
    int n = std::max(static_cast<int>(std::ceil(len / interval)), 2);
    for (int i = 0; i <= n; ++i) {
        float t = float(i) / float(n);
        appendUnique(out, (1-t)*(1-t)*p0 + 2*(1-t)*t*p1 + t*t*p2);
    }
}

// Generate the clothoid transition curve for one bend (p0→p1→p2).
// p0 and p2 are the edge-midpoints flanking the bend vertex p1.
void appendClothoidBend(std::vector<v3>& out, v3 p0, v3 p1, v3 p2, float interval) {
    // --- degenerate checks ---
    {
        float l0 = glm::length(p1 - p0);
        float l1 = glm::length(p2 - p1);
        if (l0 < 1e-4f || l1 < 1e-4f) { appendBezier(out, p0, p1, p2, interval); return; }

        v3 d0 = (p1 - p0) / l0;
        v3 d1 = (p2 - p1) / l1;
        float cosAngle = std::clamp(glm::dot(d0, d1), -1.0f, 1.0f);
        if (glm::length(glm::cross(d0, d1)) < 0.001f && std::abs(1.0f - std::abs(cosAngle)) < 0.001f) {
            appendBezier(out, p0, p1, p2, interval); return;
        }
    }

    // --- local 2D coordinate frame ---
    Frame frame;
    if (!buildFrame(p0, p1, p2, frame)) { appendBezier(out, p0, p1, p2, interval); return; }

    v2 loc1 = { worldToLocal(frame, p1).x, worldToLocal(frame, p1).y };
    v2 loc2 = { worldToLocal(frame, p2).x, worldToLocal(frame, p2).y };

    float segLen0 = glm::length(loc1);
    float segLen1 = glm::distance(loc2, loc1);
    if (segLen0 < 1e-4f || segLen1 < 1e-4f) { appendBezier(out, p0, p1, p2, interval); return; }

    // --- turning angle ---
    v2 v0 = loc1 / segLen0;
    v2 v1 = glm::normalize(loc2 - loc1);
    float I = std::acos(std::clamp(glm::dot(v0, v1), -1.0f, 1.0f));
    if (I < 0.001f) { appendBezier(out, p0, p1, p2, interval); return; }

    // --- clothoid parameters ---
    float tau1 = I * kClothoidAngleRatio * 0.5f;
    float tau2 = tau1;
    float theta = I - (tau1 + tau2);    // central arc angle
    float A1 = std::sqrt(tau1 * 2.0f);
    float A2 = std::sqrt(tau2 * 2.0f);
    float L1 = tau1 * 2.0f;            // = A1^2
    float L2 = tau2 * 2.0f;

    // --- clothoid endpoints (in normalised clothoid space) ---
    v2 ce0 = { clothoidX(tau1, A1), clothoidY(tau1, A1) };
    v2 ce1 = { clothoidX(tau2, A2), clothoidY(tau2, A2) };

    // arc centre and endpoints
    v2 vecTau1 = { std::cos(tau1), std::sin(tau1) };
    v2 center   = ce0 + v2(-vecTau1.y, vecTau1.x);
    v2 startDir = ce0 - center;
    float startRot = std::atan2(startDir.y, startDir.x);
    float endRot   = startRot + theta;
    v2 arcEnd = v2(std::cos(endRot), std::sin(endRot)) + center;

    // mirrored start of second clothoid
    v2 mirroredStart = arcEnd + rotate2({ ce1.x, -ce1.y }, I);

    // --- fit scale/offset to actual segment lengths ---
    // Guide "control point": intersection of first-segment axis and
    // the reversed second-segment direction through mirroredStart.
    v2 guideP1 = lineIntersect2D(
        { 0.0f, 0.0f }, { 1.0f, 0.0f },
        mirroredStart, mirroredStart + (loc1 - loc2));  // direction p2→p1

    float localLen0 = std::max(glm::distance(guideP1, v2(0.0f)), 1e-4f);
    float localLen1 = std::max(glm::distance(guideP1, mirroredStart), 1e-4f);
    float localRatio = localLen0 / localLen1;
    float ratio      = segLen0 / segLen1;

    float scale = 1.0f, offset = 0.0f;
    if (ratio > localRatio) {
        scale  = segLen1 / localLen1;
        offset = segLen0 - localLen0 * scale;
    } else {
        scale = segLen0 / localLen0;
    }

    float localInterval = std::max(interval / std::max(scale, 1e-4f), 1e-4f);

    auto fitAndAppend = [&](v2 p) {
        v2 fitted = p * scale;
        if (ratio > localRatio) fitted.x += offset;
        appendUnique(out, localToWorld(frame, { fitted.x, fitted.y, 0.0f }));
    };

    // --- emit samples ---
    appendUnique(out, p0);

    // first clothoid
    int n0 = std::max(static_cast<int>(std::ceil(L1 / localInterval)), 2);
    for (int i = 0; i < n0; ++i) {
        float t = tau1 * float(i) / float(std::max(n0 - 1, 1));
        fitAndAppend({ clothoidX(t, A1), clothoidY(t, A1) });
    }

    // arc
    int na = std::max(static_cast<int>(std::ceil(theta / localInterval)), 2);
    for (int i = 1; i <= na; ++i) {
        float angle = startRot + theta * float(i) / float(na);
        fitAndAppend(v2(std::cos(angle), std::sin(angle)) + center);
    }

    // second clothoid (reversed)
    int n1 = std::max(static_cast<int>(std::ceil(L2 / localInterval)), 2);
    std::vector<v2> cl2;
    cl2.reserve(n1);
    for (int i = 0; i < n1; ++i) {
        float t = tau2 * float(i) / float(std::max(n1 - 1, 1));
        v2 s = { clothoidX(t, A2), clothoidY(t, A2) };
        cl2.push_back(arcEnd + rotate2({ ce1.x - s.x, s.y - ce1.y }, I));
    }
    std::reverse(cl2.begin(), cl2.end());
    for (const v2& p : cl2) fitAndAppend(p);

    appendUnique(out, p2);
}

// ---------------------------------------------------------------------------
// Detailed versions — same geometry, each point tagged with SegKind
// ---------------------------------------------------------------------------

void appendUniqueD(std::vector<CurvePt>& out, v3 p, SegKind k) {
    if (!out.empty()) {
        v3 d = out.back().pos - p;
        if (glm::dot(d, d) < 1e-8f) return;
    }
    out.push_back({p, k});
}

void appendBezierDetailed(std::vector<CurvePt>& out, v3 p0, v3 p1, v3 p2, float interval) {
    float len = glm::length(p1 - p0) + glm::length(p2 - p1);
    int n = std::max(static_cast<int>(std::ceil(len / interval)), 2);
    for (int i = 0; i <= n; ++i) {
        float t = float(i) / float(n);
        appendUniqueD(out, (1-t)*(1-t)*p0 + 2*(1-t)*t*p1 + t*t*p2, SegKind::Straight);
    }
}

void appendClothoidBendDetailed(std::vector<CurvePt>& out, v3 p0, v3 p1, v3 p2, float interval) {
    // --- degenerate checks (same as non-detailed version) ---
    {
        float l0 = glm::length(p1 - p0);
        float l1 = glm::length(p2 - p1);
        if (l0 < 1e-4f || l1 < 1e-4f) { appendBezierDetailed(out, p0, p1, p2, interval); return; }
        v3 d0 = (p1 - p0) / l0, d1 = (p2 - p1) / l1;
        float cosA = std::clamp(glm::dot(d0, d1), -1.0f, 1.0f);
        if (glm::length(glm::cross(d0, d1)) < 0.001f && std::abs(1.0f - std::abs(cosA)) < 0.001f) {
            appendBezierDetailed(out, p0, p1, p2, interval); return;
        }
    }

    Frame frame;
    if (!buildFrame(p0, p1, p2, frame)) { appendBezierDetailed(out, p0, p1, p2, interval); return; }

    v2 loc1 = { worldToLocal(frame, p1).x, worldToLocal(frame, p1).y };
    v2 loc2 = { worldToLocal(frame, p2).x, worldToLocal(frame, p2).y };
    float segLen0 = glm::length(loc1);
    float segLen1 = glm::distance(loc2, loc1);
    if (segLen0 < 1e-4f || segLen1 < 1e-4f) { appendBezierDetailed(out, p0, p1, p2, interval); return; }

    v2 v0 = loc1 / segLen0;
    v2 v1 = glm::normalize(loc2 - loc1);
    float I = std::acos(std::clamp(glm::dot(v0, v1), -1.0f, 1.0f));
    if (I < 0.001f) { appendBezierDetailed(out, p0, p1, p2, interval); return; }

    float tau1 = I * kClothoidAngleRatio * 0.5f;
    float tau2 = tau1;
    float theta = I - (tau1 + tau2);
    float A1 = std::sqrt(tau1 * 2.0f), A2 = std::sqrt(tau2 * 2.0f);
    float L1 = tau1 * 2.0f, L2 = tau2 * 2.0f;

    v2 ce0 = { clothoidX(tau1, A1), clothoidY(tau1, A1) };
    v2 ce1 = { clothoidX(tau2, A2), clothoidY(tau2, A2) };
    v2 vecTau1 = { std::cos(tau1), std::sin(tau1) };
    v2 center  = ce0 + v2(-vecTau1.y, vecTau1.x);
    v2 startDir = ce0 - center;
    float startRot = std::atan2(startDir.y, startDir.x);
    float endRot   = startRot + theta;
    v2 arcEnd = v2(std::cos(endRot), std::sin(endRot)) + center;
    v2 mirroredStart = arcEnd + rotate2({ ce1.x, -ce1.y }, I);

    v2 guideP1 = lineIntersect2D({0,0}, {1,0}, mirroredStart, mirroredStart + (loc1 - loc2));
    float localLen0 = std::max(glm::distance(guideP1, v2(0.0f)), 1e-4f);
    float localLen1 = std::max(glm::distance(guideP1, mirroredStart), 1e-4f);
    float localRatio = localLen0 / localLen1;
    float ratio      = segLen0 / segLen1;

    float scale = 1.0f, offset = 0.0f;
    if (ratio > localRatio) {
        scale  = segLen1 / localLen1;
        offset = segLen0 - localLen0 * scale;
    } else {
        scale = segLen0 / localLen0;
    }

    float localInterval = std::max(interval / std::max(scale, 1e-4f), 1e-4f);

    // fit a local 2D point into world space, tag with kind
    auto fitAndAppendD = [&](v2 p, SegKind k) {
        v2 fitted = p * scale;
        if (ratio > localRatio) fitted.x += offset;
        appendUniqueD(out, localToWorld(frame, { fitted.x, fitted.y, 0.0f }), k);
    };

    // p0 → clothoid1 start: straight if offset > 0, otherwise merges with clothoid start
    appendUniqueD(out, p0, SegKind::Straight);

    // first clothoid
    int n0 = std::max(static_cast<int>(std::ceil(L1 / localInterval)), 2);
    for (int i = 0; i < n0; ++i) {
        float t = tau1 * float(i) / float(std::max(n0 - 1, 1));
        fitAndAppendD({ clothoidX(t, A1), clothoidY(t, A1) }, SegKind::Clothoid);
    }

    // arc
    int na = std::max(static_cast<int>(std::ceil(theta / localInterval)), 2);
    for (int i = 1; i <= na; ++i) {
        float angle = startRot + theta * float(i) / float(na);
        fitAndAppendD(v2(std::cos(angle), std::sin(angle)) + center, SegKind::Arc);
    }

    // second clothoid (reversed) — collect then reverse
    int n1 = std::max(static_cast<int>(std::ceil(L2 / localInterval)), 2);
    std::vector<v2> cl2;
    cl2.reserve(n1);
    for (int i = 0; i < n1; ++i) {
        float t = tau2 * float(i) / float(std::max(n1 - 1, 1));
        v2 s = { clothoidX(t, A2), clothoidY(t, A2) };
        cl2.push_back(arcEnd + rotate2({ ce1.x - s.x, s.y - ce1.y }, I));
    }
    std::reverse(cl2.begin(), cl2.end());
    for (const v2& p : cl2) fitAndAppendD(p, SegKind::Clothoid);

    // clothoid2 end → p2: straight
    appendUniqueD(out, p2, SegKind::Straight);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
namespace ClothoidGen {

std::vector<glm::vec3> buildCenterline(
    const std::vector<ControlPoint>& pts,
    float sampleInterval,
    bool  equalMidpoint)
{
    using v3 = glm::vec3;
    std::vector<v3> out;
    int n = static_cast<int>(pts.size());
    if (n < 2) return out;
    if (n == 2) {
        out.push_back(pts[0].pos);
        out.push_back(pts[1].pos);
        return out;
    }

    // Compute edge-midpoints.
    // equalMidpoint=true  → t=0.5 (true midpoint of each edge)
    // equalMidpoint=false → t = prevEdgeLen / (prevEdgeLen + thisEdgeLen)
    std::vector<v3> mid(n - 1);
    for (int e = 0; e < n - 1; ++e) {
        float t = 0.5f;
        if (!equalMidpoint) {
            float prevLen = (e > 0)
                ? glm::distance(pts[e - 1].pos, pts[e].pos)
                : 0.0f;
            float thisLen = glm::distance(pts[e].pos, pts[e + 1].pos);
            float sum = prevLen + thisLen;
            t = (sum > 1e-5f) ? prevLen / sum : 0.5f;
        }
        mid[e] = glm::mix(pts[e].pos, pts[e + 1].pos, t);
    }

    // p[0] to first midpoint: straight section, no clothoid needed yet
    appendUnique(out, pts[0].pos);

    // clothoid bend at each intermediate control point
    for (int i = 1; i < n - 1; ++i) {
        appendClothoidBend(out, mid[i - 1], pts[i].pos, mid[i], sampleInterval);
    }

    // last midpoint to p[n-1]
    appendUnique(out, pts[n - 1].pos);

    return out;
}

std::vector<glm::vec3> resamplePolyline(
    const std::vector<glm::vec3>& pts,
    float interval)
{
    using v3 = glm::vec3;
    std::vector<v3> out;
    if (pts.size() < 2 || interval <= 0.0f) return out;

    out.push_back(pts.front());

    // Walk along the polyline, accumulating arc length.
    float accumulated = 0.0f;
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        v3 a = pts[i];
        v3 b = pts[i + 1];
        float segLen = glm::distance(a, b);
        if (segLen < 1e-6f) continue;

        float remaining = interval - accumulated;
        while (remaining <= segLen) {
            float t = remaining / segLen;
            out.push_back(glm::mix(a, b, t));
            a          = out.back();
            segLen    -= remaining;
            accumulated = 0.0f;
            remaining   = interval;
        }
        accumulated += segLen;
    }

    // Always include the last point unless it's essentially the same as the previous
    v3 last = pts.back();
    if (glm::distance(out.back(), last) > 1e-4f)
        out.push_back(last);

    return out;
}

std::vector<glm::vec3> buildAndResample(
    const std::vector<ControlPoint>& pts,
    float interval,
    bool  equalMidpoint)
{
    // Sample clothoid at 1/10 of the target interval for accuracy
    float fineInterval = std::max(interval / 10.0f, 0.01f);
    auto centerline = buildCenterline(pts, fineInterval, equalMidpoint);
    return resamplePolyline(centerline, interval);
}

std::vector<CurvePt> buildCenterlineDetailed(
    const std::vector<ControlPoint>& pts,
    float sampleInterval,
    bool  equalMidpoint)
{
    using v3 = glm::vec3;
    std::vector<CurvePt> out;
    int n = static_cast<int>(pts.size());
    if (n < 2) return out;
    if (n == 2) {
        out.push_back({pts[0].pos, SegKind::Straight});
        out.push_back({pts[1].pos, SegKind::Straight});
        return out;
    }

    std::vector<v3> mid(n - 1);
    for (int e = 0; e < n - 1; ++e) {
        float t = 0.5f;
        if (!equalMidpoint) {
            float prevLen = (e > 0)
                ? glm::distance(pts[e - 1].pos, pts[e].pos)
                : 0.0f;
            float thisLen = glm::distance(pts[e].pos, pts[e + 1].pos);
            float sum = prevLen + thisLen;
            t = (sum > 1e-5f) ? prevLen / sum : 0.5f;
        }
        mid[e] = glm::mix(pts[e].pos, pts[e + 1].pos, t);
    }

    appendUniqueD(out, pts[0].pos, SegKind::Straight);

    for (int i = 1; i < n - 1; ++i) {
        appendClothoidBendDetailed(out, mid[i - 1], pts[i].pos, mid[i], sampleInterval);
    }

    appendUniqueD(out, pts[n - 1].pos, SegKind::Straight);

    return out;
}

std::vector<CurvePt> buildCenterlineDetailedResampled(
    const std::vector<ControlPoint>& pts,
    float interval,
    bool  equalMidpoint)
{
    std::vector<CurvePt> out;
    if (pts.size() < 2)
        return out;

    const float targetInterval = std::max(interval, 0.01f);
    const float fineInterval = std::max(targetInterval / 10.0f, 0.01f);
    auto fineCurve = buildCenterlineDetailed(pts, fineInterval, equalMidpoint);
    if (fineCurve.size() < 2)
        return fineCurve;

    out.push_back(fineCurve.front());

    float accumulated = 0.0f;
    for (size_t i = 0; i + 1 < fineCurve.size(); ++i) {
        CurvePt a = fineCurve[i];
        CurvePt b = fineCurve[i + 1];
        float segLen = glm::distance(a.pos, b.pos);
        if (segLen < 1e-6f)
            continue;

        float remaining = targetInterval - accumulated;
        while (remaining <= segLen) {
            float t = remaining / segLen;
            CurvePt sample;
            sample.pos = glm::mix(a.pos, b.pos, t);
            sample.kind = a.kind;
            sample.verticalKind = a.verticalKind;
            out.push_back(sample);
            a = sample;
            segLen -= remaining;
            accumulated = 0.0f;
            remaining = targetInterval;
        }
        accumulated += segLen;
    }

    if (glm::distance(out.back().pos, fineCurve.back().pos) > 1e-4f)
        out.push_back(fineCurve.back());

    return out;
}

} // namespace ClothoidGen
