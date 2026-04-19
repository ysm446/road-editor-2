#include "Serializer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <QDebug>

using json = nlohmann::json;

static glm::vec3 readVec3(const json& j) {
    return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
}

static json writeVec3(const glm::vec3& v) {
    return json::array({v.x, v.y, v.z});
}

static RoadEndpointLink readLink(const json& j) {
    RoadEndpointLink link;
    if (!j.is_object()) return link;
    link.intersectionId = j.value("intersectionId", "");
    link.socketId = j.value("socketId", "");
    return link;
}

static json writeLink(const RoadEndpointLink& link) {
    return {
        {"intersectionId", link.intersectionId},
        {"socketId", link.socketId}
    };
}

static IntersectionSocket readSocket(const json& j) {
    IntersectionSocket socket;
    socket.id = j.value("id", "");
    socket.name = j.value("name", "");
    if (j.contains("localPos")) socket.localPos = readVec3(j["localPos"]);
    socket.yaw = j.value("yaw", 0.0f);
    socket.enabled = j.value("enabled", true);
    return socket;
}

static json writeSocket(const IntersectionSocket& socket) {
    return {
        {"id", socket.id},
        {"name", socket.name},
        {"localPos", writeVec3(socket.localPos)},
        {"yaw", socket.yaw},
        {"enabled", socket.enabled}
    };
}

static std::vector<IntersectionSocket> defaultSockets(float entryDist) {
    const float radius = std::max(entryDist, 12.0f);
    return {
        {"s0", "East",  { radius, 0.0f, 0.0f}, 0.0f, true},
        {"s1", "West",  {-radius, 0.0f, 0.0f}, 3.1415926f, true},
        {"s2", "North", {0.0f, 0.0f,  radius}, 1.5707963f, true},
        {"s3", "South", {0.0f, 0.0f, -radius}, -1.5707963f, true}
    };
}

bool Serializer::loadFromFile(const QString& path, RoadNetwork& net) {
    std::ifstream f(path.toStdString());
    if (!f.is_open()) {
        qWarning() << "Cannot open:" << path;
        return false;
    }

    json doc;
    try { f >> doc; }
    catch (const json::exception& e) {
        qWarning() << "JSON parse error:" << e.what();
        return false;
    }

    net.clear();
    net.version = doc.value("version", 3);

    for (const auto& jg : doc.value("groups", json::array())) {
        Group g;
        g.id      = jg.value("id",      "");
        g.name    = jg.value("name",    "");
        g.locked  = jg.value("locked",  false);
        g.visible = jg.value("visible", true);
        net.groups.push_back(std::move(g));
    }

    for (const auto& ji : doc.value("intersections", json::array())) {
        Intersection n;
        n.id        = ji.value("id",        "");
        n.name      = ji.value("name",      "");
        n.groupId   = ji.value("groupId",   "");
        n.type      = ji.value("type",      "intersection");
        n.entryDist = ji.value("entryDist", 8.0f);
        if (ji.contains("pos")) n.pos = readVec3(ji["pos"]);
        for (const auto& js : ji.value("sockets", json::array()))
            n.sockets.push_back(readSocket(js));
        if (n.sockets.empty())
            n.sockets = defaultSockets(n.entryDist);
        net.intersections.push_back(std::move(n));
    }

    for (const auto& jr : doc.value("roads", json::array())) {
        Road r;
        r.id                     = jr.value("id",                     "");
        r.name                   = jr.value("name",                   "");
        r.groupId                = jr.value("groupId",                "");
        r.startIntersectionId    = jr.value("startIntersectionId",    "");
        r.endIntersectionId      = jr.value("endIntersectionId",      "");
        if (jr.contains("startLink")) r.startLink = readLink(jr["startLink"]);
        if (jr.contains("endLink"))   r.endLink   = readLink(jr["endLink"]);
        r.defaultTargetSpeed     = jr.value("defaultTargetSpeed",     40.0f);
        r.defaultFriction        = jr.value("defaultFriction",        0.15f);
        r.useLaneLeft2           = jr.value("useLaneLeft2",           false);
        r.defaultWidthLaneLeft2  = jr.value("defaultWidthLaneLeft2",  3.5f);
        r.useLaneLeft1           = jr.value("useLaneLeft1",           true);
        r.defaultWidthLaneLeft1  = jr.value("defaultWidthLaneLeft1",  4.0f);
        r.useLaneCenter          = jr.value("useLaneCenter",          false);
        r.defaultWidthLaneCenter = jr.value("defaultWidthLaneCenter", 0.0f);
        r.useLaneRight1          = jr.value("useLaneRight1",          true);
        r.defaultWidthLaneRight1 = jr.value("defaultWidthLaneRight1", 4.0f);
        r.useLaneRight2          = jr.value("useLaneRight2",          false);
        r.defaultWidthLaneRight2 = jr.value("defaultWidthLaneRight2", 3.5f);
        r.roadType               = jr.value("roadType",               0);
        r.closed                 = jr.value("closed",                 false);
        r.active                 = jr.value("active",                 1);

        for (const auto& jp : jr.value("point", json::array())) {
            ControlPoint cp;
            if (jp.contains("pos")) cp.pos = readVec3(jp["pos"]);
            cp.curvatureRadius     = jp.value("curvatureRadius",     0.0f);
            cp.useCurvatureRadius  = jp.value("useCurvatureRadius",  0);
            r.points.push_back(cp);
        }

        if (!r.startIntersectionId.empty() && r.startLink.intersectionId.empty())
            r.startLink.intersectionId = r.startIntersectionId;
        if (!r.endIntersectionId.empty() && r.endLink.intersectionId.empty())
            r.endLink.intersectionId = r.endIntersectionId;

        net.roads.push_back(std::move(r));
    }

    qDebug() << "Loaded:" << net.roads.size() << "roads,"
             << net.intersections.size() << "intersections from" << path;
    return true;
}

bool Serializer::saveToFile(const QString& path, const RoadNetwork& net) {
    json doc;
    doc["version"] = net.version;

    json jGroups = json::array();
    for (const auto& g : net.groups) {
        jGroups.push_back({{"id", g.id}, {"name", g.name},
                           {"locked", g.locked}, {"visible", g.visible}});
    }
    doc["groups"] = jGroups;

    json jIntersections = json::array();
    for (const auto& n : net.intersections) {
        json jIntersection = {
            {"id", n.id}, {"name", n.name}, {"groupId", n.groupId},
            {"type", n.type}, {"entryDist", n.entryDist},
            {"pos", writeVec3(n.pos)},
            {"sockets", json::array()}
        };
        for (const auto& socket : n.sockets)
            jIntersection["sockets"].push_back(writeSocket(socket));
        jIntersections.push_back(std::move(jIntersection));
    }
    doc["intersections"] = jIntersections;

    json jRoads = json::array();
    for (const auto& r : net.roads) {
        json jr = {
            {"id", r.id}, {"name", r.name}, {"groupId", r.groupId},
            {"startIntersectionId", r.startIntersectionId},
            {"endIntersectionId",   r.endIntersectionId},
            {"startLink", writeLink(r.startLink)},
            {"endLink",   writeLink(r.endLink)},
            {"defaultTargetSpeed",     r.defaultTargetSpeed},
            {"defaultFriction",        r.defaultFriction},
            {"useLaneLeft2",           r.useLaneLeft2},
            {"defaultWidthLaneLeft2",  r.defaultWidthLaneLeft2},
            {"useLaneLeft1",           r.useLaneLeft1},
            {"defaultWidthLaneLeft1",  r.defaultWidthLaneLeft1},
            {"useLaneCenter",          r.useLaneCenter},
            {"defaultWidthLaneCenter", r.defaultWidthLaneCenter},
            {"useLaneRight1",          r.useLaneRight1},
            {"defaultWidthLaneRight1", r.defaultWidthLaneRight1},
            {"useLaneRight2",          r.useLaneRight2},
            {"defaultWidthLaneRight2", r.defaultWidthLaneRight2},
            {"roadType", r.roadType}, {"closed", r.closed}, {"active", r.active},
            {"verticalCurve", json::array()},
            {"bankAngle",     json::array()},
            {"laneSections",  json::array()}
        };

        json jPoints = json::array();
        for (const auto& cp : r.points) {
            jPoints.push_back({
                {"pos", writeVec3(cp.pos)},
                {"curvatureRadius",    cp.curvatureRadius},
                {"useCurvatureRadius", cp.useCurvatureRadius}
            });
        }
        jr["point"] = jPoints;
        jRoads.push_back(jr);
    }
    doc["roads"] = jRoads;

    std::ofstream f(path.toStdString());
    if (!f.is_open()) {
        qWarning() << "Cannot write:" << path;
        return false;
    }
    f << doc.dump(2);
    return true;
}
