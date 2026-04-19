#include "VerticalCurveGen.h"
#include <algorithm>
#include <cmath>

namespace {

struct GuidePoint {
    float x = 0.0f;
    float y = 0.0f;
    float vcl = 0.0f;
};

struct Segment {
    GuidePoint p0;
    GuidePoint p1;
    GuidePoint p2;
    bool validCurve = false;
    VerticalSegKind curveKind = VerticalSegKind::Other;
    float slopeIn = 0.0f;
    float slopeOut = 0.0f;
    float curveLength = 0.0f;
    float curveStartX = 0.0f;
    float curveStartY = 0.0f;
    float curveEndX = 0.0f;
    float curveEndY = 0.0f;
};

std::vector<float> buildArcLengths(const std::vector<glm::vec3>& pts) {
    if (pts.empty()) return {};
    std::vector<float> out;
    out.reserve(pts.size());
    out.push_back(0.0f);
    for (size_t i = 1; i < pts.size(); ++i)
        out.push_back(out.back() + glm::length(pts[i] - pts[i - 1]));
    return out;
}

glm::vec3 sampleAtDistance(
    const std::vector<glm::vec3>& pts,
    const std::vector<float>& arc,
    float distance) {
    if (pts.empty()) return {};
    if (pts.size() == 1) return pts.front();
    if (distance <= 0.0f) return pts.front();
    if (distance >= arc.back()) return pts.back();

    auto it = std::lower_bound(arc.begin(), arc.end(), distance);
    size_t i = static_cast<size_t>(std::max<std::ptrdiff_t>(1, it - arc.begin())) - 1;
    float d0 = arc[i];
    float d1 = arc[i + 1];
    float span = std::max(d1 - d0, 1e-6f);
    float t = std::clamp((distance - d0) / span, 0.0f, 1.0f);
    return glm::mix(pts[i], pts[i + 1], t);
}

Segment buildSegment(GuidePoint p0, GuidePoint p1, GuidePoint p2) {
    Segment seg;
    seg.p0 = p0;
    seg.p1 = p1;
    seg.p2 = p2;

    const float eps = 1e-5f;
    const float dx0 = p1.x - p0.x;
    const float dx1 = p2.x - p1.x;
    if (std::abs(dx0) < eps || std::abs(dx1) < eps)
        return seg;

    const float i1 = (p1.y - p0.y) / dx0;
    const float i2 = (p2.y - p1.y) / dx1;
    float L = std::max(0.0f, p1.vcl);
    L = std::min(L, std::abs(dx0) * 2.0f);
    L = std::min(L, std::abs(dx1) * 2.0f);
    if (L < eps)
        return seg;

    seg.validCurve = true;
    seg.slopeIn = i1;
    seg.slopeOut = i2;
    seg.curveLength = L;
    seg.curveStartX = p1.x - L * 0.5f;
    seg.curveStartY = p0.y + i1 * (seg.curveStartX - p0.x);
    seg.curveEndX = seg.curveStartX + L;
    seg.curveEndY = seg.curveStartY + i1 * L - ((i1 - i2) / (2.0f * L)) * L * L;
    seg.curveKind = (i2 > i1) ? VerticalSegKind::Sag : VerticalSegKind::Crest;
    return seg;
}

float sampleHeight(const Segment& seg, float distance) {
    if (!seg.validCurve) {
        float x0 = seg.p0.x;
        float x2 = seg.p2.x;
        if (x2 - x0 <= 1e-5f)
            return seg.p1.y;

        float t = std::clamp((distance - x0) / (x2 - x0), 0.0f, 1.0f);
        float omt = 1.0f - t;
        return omt * omt * seg.p0.y + 2.0f * omt * t * seg.p1.y + t * t * seg.p2.y;
    }

    if (distance <= seg.curveStartX)
        return seg.p0.y + seg.slopeIn * (distance - seg.p0.x);
    if (distance >= seg.curveEndX)
        return seg.p2.y - seg.slopeOut * (seg.p2.x - distance);

    const float x = distance - seg.curveStartX;
    return seg.curveStartY + seg.slopeIn * x -
        ((seg.slopeIn - seg.slopeOut) / (2.0f * seg.curveLength)) * x * x;
}

VerticalSegKind sampleVerticalKind(const Segment& seg, float distance) {
    if (!seg.validCurve)
        return VerticalSegKind::Other;
    if (distance >= seg.curveStartX && distance <= seg.curveEndX)
        return seg.curveKind;
    return VerticalSegKind::Other;
}

template <typename PointT>
std::vector<PointT> applyImpl(
    const Road& road,
    const std::vector<PointT>& baseCurve,
    float sampleInterval) {
    std::vector<PointT> samples;
    if (baseCurve.size() < 2)
        return samples;

    std::vector<glm::vec3> basePositions;
    basePositions.reserve(baseCurve.size());
    for (const auto& p : baseCurve)
        basePositions.push_back(p.pos);

    const std::vector<float> arc = buildArcLengths(basePositions);
    if (arc.empty())
        return samples;

    const float totalLength = arc.back();
    if (totalLength <= 1e-5f)
        return samples;

    if (road.verticalCurve.empty()) {
        return baseCurve;
    }

    std::vector<VerticalCurvePoint> points = road.verticalCurve;
    std::sort(points.begin(), points.end(),
              [](const VerticalCurvePoint& a, const VerticalCurvePoint& b) { return a.u < b.u; });

    std::vector<GuidePoint> guide;
    guide.reserve(points.size() + 2);
    guide.push_back({0.0f, basePositions.front().y, 0.0f});

    for (const auto& curvePoint : points) {
        float distance = std::clamp(curvePoint.u, 0.0f, 1.0f) * totalLength;
        glm::vec3 basePoint = sampleAtDistance(basePositions, arc, distance);
        guide.push_back({distance, basePoint.y + curvePoint.offset, curvePoint.vcl});
    }

    guide.push_back({totalLength, basePositions.back().y, 0.0f});
    if (guide.size() < 3)
        return baseCurve;

    std::vector<Segment> segments;
    segments.reserve(guide.size() - 2);
    for (size_t i = 1; i + 1 < guide.size(); ++i) {
        bool isFirst = (i == 1);
        bool isLast = (i + 2 == guide.size());
        GuidePoint start = isFirst
            ? guide.front()
            : GuidePoint{(guide[i - 1].x + guide[i].x) * 0.5f, (guide[i - 1].y + guide[i].y) * 0.5f, 0.0f};
        GuidePoint end = isLast
            ? guide.back()
            : GuidePoint{(guide[i].x + guide[i + 1].x) * 0.5f, (guide[i].y + guide[i + 1].y) * 0.5f, 0.0f};
        segments.push_back(buildSegment(start, guide[i], end));
    }

    const float step = std::max(sampleInterval, 1.0f);
    for (size_t si = 0; si < segments.size(); ++si) {
        const auto& seg = segments[si];
        float len = seg.p2.x - seg.p0.x;
        if (len <= 1e-5f)
            continue;
        int sampleCount = std::max(static_cast<int>(std::ceil(len / step)), 2);
        int startStep = (si == 0) ? 0 : 1;
        for (int i = startStep; i <= sampleCount; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(sampleCount);
            float distance = seg.p0.x + (seg.p2.x - seg.p0.x) * t;
            glm::vec3 pos = sampleAtDistance(basePositions, arc, distance);
            pos.y = sampleHeight(seg, distance);

            PointT out{};
            out.pos = pos;
            size_t baseIdx = std::min(baseCurve.size() - 1, static_cast<size_t>(
                std::lower_bound(arc.begin(), arc.end(), distance) - arc.begin()));
            out.kind = baseCurve[baseIdx].kind;
            out.verticalKind = sampleVerticalKind(seg, distance);
            samples.push_back(out);
        }
    }

    return samples;
}

} // namespace

namespace VerticalCurveGen {

std::vector<glm::vec3> apply(
    const Road& road,
    const std::vector<glm::vec3>& baseCurve,
    float sampleInterval) {
    std::vector<CurvePt> detailed;
    detailed.reserve(baseCurve.size());
    for (const auto& pos : baseCurve)
        detailed.push_back({pos, SegKind::Straight});
    auto adjusted = applyImpl(road, detailed, sampleInterval);
    std::vector<glm::vec3> out;
    out.reserve(adjusted.size());
    for (const auto& p : adjusted)
        out.push_back(p.pos);
    return out;
}

std::vector<CurvePt> applyDetailed(
    const Road& road,
    const std::vector<CurvePt>& baseCurve,
    float sampleInterval) {
    return applyImpl(road, baseCurve, sampleInterval);
}

} // namespace VerticalCurveGen
