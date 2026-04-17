#include "Serializer.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <QDebug>

using json = nlohmann::json;

static glm::vec3 readVec3(const json& j) {
    return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
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
        net.intersections.push_back(std::move(n));
    }

    for (const auto& jr : doc.value("roads", json::array())) {
        Road r;
        r.id                     = jr.value("id",                     "");
        r.name                   = jr.value("name",                   "");
        r.groupId                = jr.value("groupId",                "");
        r.startIntersectionId    = jr.value("startIntersectionId",    "");
        r.endIntersectionId      = jr.value("endIntersectionId",      "");
        r.defaultTargetSpeed     = jr.value("defaultTargetSpeed",     40.0f);
        r.defaultFriction        = jr.value("defaultFriction",        0.15f);
        r.defaultWidthLaneLeft1  = jr.value("defaultWidthLaneLeft1",  4.0f);
        r.defaultWidthLaneRight1 = jr.value("defaultWidthLaneRight1", 4.0f);
        r.defaultWidthLaneCenter = jr.value("defaultWidthLaneCenter", 0.0f);
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
        jIntersections.push_back({
            {"id", n.id}, {"name", n.name}, {"groupId", n.groupId},
            {"type", n.type}, {"entryDist", n.entryDist},
            {"pos", {n.pos.x, n.pos.y, n.pos.z}}
        });
    }
    doc["intersections"] = jIntersections;

    json jRoads = json::array();
    for (const auto& r : net.roads) {
        json jr = {
            {"id", r.id}, {"name", r.name}, {"groupId", r.groupId},
            {"startIntersectionId", r.startIntersectionId},
            {"endIntersectionId",   r.endIntersectionId},
            {"defaultTargetSpeed",     r.defaultTargetSpeed},
            {"defaultFriction",        r.defaultFriction},
            {"defaultWidthLaneLeft1",  r.defaultWidthLaneLeft1},
            {"defaultWidthLaneRight1", r.defaultWidthLaneRight1},
            {"defaultWidthLaneCenter", r.defaultWidthLaneCenter},
            {"roadType", r.roadType}, {"closed", r.closed}, {"active", r.active},
            {"verticalCurve", json::array()},
            {"bankAngle",     json::array()},
            {"laneSections",  json::array()}
        };

        json jPoints = json::array();
        for (const auto& cp : r.points) {
            jPoints.push_back({
                {"pos", {cp.pos.x, cp.pos.y, cp.pos.z}},
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
