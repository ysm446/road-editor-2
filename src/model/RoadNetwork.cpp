#include "RoadNetwork.h"
#include <algorithm>

void RoadNetwork::clear() {
    roads.clear();
    intersections.clear();
    groups.clear();
}

const Road* RoadNetwork::findRoad(const std::string& id) const {
    auto it = std::find_if(roads.begin(), roads.end(),
                           [&](const Road& r) { return r.id == id; });
    return it != roads.end() ? &*it : nullptr;
}

const Intersection* RoadNetwork::findIntersection(const std::string& id) const {
    auto it = std::find_if(intersections.begin(), intersections.end(),
                           [&](const Intersection& n) { return n.id == id; });
    return it != intersections.end() ? &*it : nullptr;
}

Intersection* RoadNetwork::findIntersection(const std::string& id) {
    auto it = std::find_if(intersections.begin(), intersections.end(),
                           [&](const Intersection& n) { return n.id == id; });
    return it != intersections.end() ? &*it : nullptr;
}

const IntersectionSocket* RoadNetwork::findSocket(
    const std::string& intersectionId, const std::string& socketId) const {
    const Intersection* ix = findIntersection(intersectionId);
    if (!ix) return nullptr;

    auto it = std::find_if(ix->sockets.begin(), ix->sockets.end(),
                           [&](const IntersectionSocket& socket) {
                               return socket.id == socketId;
                           });
    return it != ix->sockets.end() ? &*it : nullptr;
}

IntersectionSocket* RoadNetwork::findSocket(
    const std::string& intersectionId, const std::string& socketId) {
    Intersection* ix = findIntersection(intersectionId);
    if (!ix) return nullptr;

    auto it = std::find_if(ix->sockets.begin(), ix->sockets.end(),
                           [&](const IntersectionSocket& socket) {
                               return socket.id == socketId;
                           });
    return it != ix->sockets.end() ? &*it : nullptr;
}
