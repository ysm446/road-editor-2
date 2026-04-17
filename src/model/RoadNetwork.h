#pragma once

#include "Road.h"

struct RoadNetwork {
    std::vector<Road>         roads;
    std::vector<Intersection> intersections;
    std::vector<Group>        groups;
    int version = 3;

    void clear();
    const Road*         findRoad        (const std::string& id) const;
    const Intersection* findIntersection(const std::string& id) const;
};
