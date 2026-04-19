#pragma once

#include "../model/Road.h"

struct EvaluatedLaneSection {
    float offsetCenter = 0.0f;
    float widthLeft2 = 0.0f;
    float widthLeft1 = 0.0f;
    float widthCenter = 0.0f;
    float widthRight1 = 0.0f;
    float widthRight2 = 0.0f;
};

class LaneSectionGen {
public:
    static EvaluatedLaneSection evaluateAtU(const Road& road, float u);
};
