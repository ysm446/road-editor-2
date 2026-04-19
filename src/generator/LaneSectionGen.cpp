#include "LaneSectionGen.h"
#include <algorithm>

namespace {
EvaluatedLaneSection fromDefaults(const Road& road) {
    EvaluatedLaneSection section;
    section.widthLeft2 = road.useLaneLeft2 ? road.defaultWidthLaneLeft2 : 0.0f;
    section.widthLeft1 = road.useLaneLeft1 ? road.defaultWidthLaneLeft1 : 0.0f;
    section.widthCenter = road.useLaneCenter ? road.defaultWidthLaneCenter : 0.0f;
    section.widthRight1 = road.useLaneRight1 ? road.defaultWidthLaneRight1 : 0.0f;
    section.widthRight2 = road.useLaneRight2 ? road.defaultWidthLaneRight2 : 0.0f;
    return section;
}

EvaluatedLaneSection fromPoint(const LaneSectionPoint& point) {
    EvaluatedLaneSection section;
    section.offsetCenter = point.offsetCenter;
    section.widthLeft2 = point.useLaneLeft2 ? point.widthLaneLeft2 : 0.0f;
    section.widthLeft1 = point.useLaneLeft1 ? point.widthLaneLeft1 : 0.0f;
    section.widthCenter = point.useLaneCenter ? point.widthLaneCenter : 0.0f;
    section.widthRight1 = point.useLaneRight1 ? point.widthLaneRight1 : 0.0f;
    section.widthRight2 = point.useLaneRight2 ? point.widthLaneRight2 : 0.0f;
    return section;
}

EvaluatedLaneSection lerpSection(const EvaluatedLaneSection& a,
                                 const EvaluatedLaneSection& b,
                                 float t) {
    EvaluatedLaneSection section;
    section.offsetCenter = a.offsetCenter + (b.offsetCenter - a.offsetCenter) * t;
    section.widthLeft2 = a.widthLeft2 + (b.widthLeft2 - a.widthLeft2) * t;
    section.widthLeft1 = a.widthLeft1 + (b.widthLeft1 - a.widthLeft1) * t;
    section.widthCenter = a.widthCenter + (b.widthCenter - a.widthCenter) * t;
    section.widthRight1 = a.widthRight1 + (b.widthRight1 - a.widthRight1) * t;
    section.widthRight2 = a.widthRight2 + (b.widthRight2 - a.widthRight2) * t;
    return section;
}
}

EvaluatedLaneSection LaneSectionGen::evaluateAtU(const Road& road, float u) {
    const float clampedU = std::clamp(u, 0.0f, 1.0f);
    if (road.laneSections.empty())
        return fromDefaults(road);

    std::vector<LaneSectionPoint> sections = road.laneSections;
    std::sort(sections.begin(), sections.end(),
              [](const LaneSectionPoint& a, const LaneSectionPoint& b) { return a.u < b.u; });

    if (clampedU <= sections.front().u)
        return fromPoint(sections.front());
    if (clampedU >= sections.back().u)
        return fromPoint(sections.back());

    for (size_t i = 1; i < sections.size(); ++i) {
        if (clampedU > sections[i].u)
            continue;
        const float span = std::max(sections[i].u - sections[i - 1].u, 1e-6f);
        const float t = std::clamp((clampedU - sections[i - 1].u) / span, 0.0f, 1.0f);
        return lerpSection(fromPoint(sections[i - 1]), fromPoint(sections[i]), t);
    }

    return fromPoint(sections.back());
}
