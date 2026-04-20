#pragma once

#include "Road.h"

struct TerrainSettings {
    bool        enabled   = false;
    bool        visible   = true;
    std::string path;
    std::string texturePath;
    float       width     = 1024.0f;
    float       depth     = 1024.0f;
    float       height    = 128.0f;
    glm::vec3   offset    = {0.0f, 0.0f, 0.0f};
    int         meshCellsX = 0;
    int         meshCellsZ = 0;

    void clear() {
        enabled = false;
        visible = true;
        path.clear();
        texturePath.clear();
        width = 1024.0f;
        depth = 1024.0f;
        height = 128.0f;
        offset = {0.0f, 0.0f, 0.0f};
        meshCellsX = 0;
        meshCellsZ = 0;
    }
};

struct RoadNetwork {
    std::vector<Road>         roads;
    std::vector<Intersection> intersections;
    std::vector<Group>        groups;
    TerrainSettings           terrain;
    int version = 4;

    void clear();
    const Road*               findRoad        (const std::string& id) const;
    const Intersection*       findIntersection(const std::string& id) const;
    Intersection*             findIntersection(const std::string& id);
    const IntersectionSocket* findSocket(
        const std::string& intersectionId, const std::string& socketId) const;
    IntersectionSocket* findSocket(
        const std::string& intersectionId, const std::string& socketId);
};
