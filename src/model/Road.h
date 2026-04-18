#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct ControlPoint {
    glm::vec3 pos              = {0.0f, 0.0f, 0.0f};
    float     curvatureRadius  = 0.0f;
    int       useCurvatureRadius = 0;
};

struct VerticalCurvePoint {
    float u      = 0.0f;
    float vcl    = 0.0f; // vertical curve length
    float offset = 0.0f;
};

struct BankAnglePoint {
    float u           = 0.0f;
    float angle       = 0.0f;
    float targetSpeed = 40.0f;
    int   useAngle    = 0;
};

struct LaneSectionPoint {
    float u = 0.0f;
    // Extended when lane section format is finalised
};

struct Road {
    std::string id, name, groupId;
    std::string startIntersectionId, endIntersectionId;

    std::vector<ControlPoint>       points;
    std::vector<VerticalCurvePoint> verticalCurve;
    std::vector<BankAnglePoint>     bankAngle;
    std::vector<LaneSectionPoint>   laneSections;

    float defaultTargetSpeed     = 40.0f;
    float defaultFriction        = 0.15f;

    bool  useLaneLeft2           = false;
    float defaultWidthLaneLeft2  = 3.5f;
    bool  useLaneLeft1           = true;
    float defaultWidthLaneLeft1  = 4.0f;
    bool  useLaneCenter          = false;
    float defaultWidthLaneCenter = 0.0f;
    bool  useLaneRight1          = true;
    float defaultWidthLaneRight1 = 4.0f;
    bool  useLaneRight2          = false;
    float defaultWidthLaneRight2 = 3.5f;

    float segmentLength          = 1.0f;
    bool  equalMidpoint          = false;
    int   roadType = 0;
    bool  closed   = false;
    int   active   = 1;
};

struct Intersection {
    std::string id, name, groupId, type;
    glm::vec3   pos       = {0.0f, 0.0f, 0.0f};
    float       entryDist = 8.0f;
};

struct Group {
    std::string id, name;
    bool locked  = false;
    bool visible = true;
};
