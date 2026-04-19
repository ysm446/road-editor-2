#pragma once

#include "Road.h"

struct RoadNetwork {
    std::vector<Road>         roads;
    std::vector<Intersection> intersections;
    std::vector<Group>        groups;
    int version = 3;

    void clear();
    const Road*               findRoad        (const std::string& id) const;
    const Intersection*       findIntersection(const std::string& id) const;
    Intersection*             findIntersection(const std::string& id);
    const IntersectionSocket* findSocket(
        const std::string& intersectionId, const std::string& socketId) const;
    IntersectionSocket* findSocket(
        const std::string& intersectionId, const std::string& socketId);
};
