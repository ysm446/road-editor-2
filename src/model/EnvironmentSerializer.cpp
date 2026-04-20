#include "EnvironmentSerializer.h"
#include <nlohmann/json.hpp>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

using json = nlohmann::json;

namespace {

glm::vec3 readVec3(const json& j) {
    return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
}

json writeVec3(const glm::vec3& v) {
    return json::array({v.x, v.y, v.z});
}

QString resolvePathFromEnvironment(const QString& envPath, const QString& storedPath) {
    if (storedPath.isEmpty())
        return {};

    const QFileInfo storedInfo(storedPath);
    if (storedInfo.isAbsolute())
        return QDir::toNativeSeparators(QDir::cleanPath(storedInfo.absoluteFilePath()));

    const QFileInfo envInfo(envPath);
    const QDir baseDir(envInfo.absolutePath());
    return QDir::toNativeSeparators(
        QDir::cleanPath(baseDir.absoluteFilePath(QDir::fromNativeSeparators(storedPath))));
}

QString makePathRelativeToEnvironment(const QString& envPath, const QString& targetPath) {
    if (targetPath.isEmpty())
        return {};

    const QFileInfo targetInfo(targetPath);
    const QString normalizedTarget =
        QDir::cleanPath(targetInfo.isAbsolute() ? targetInfo.absoluteFilePath() : targetPath);

    if (envPath.isEmpty())
        return normalizedTarget;

    const QFileInfo envInfo(envPath);
    const QDir baseDir(envInfo.absolutePath());
    return QDir::cleanPath(baseDir.relativeFilePath(normalizedTarget));
}

}

bool EnvironmentSerializer::loadFromFile(const QString& path, EnvironmentState& env) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open environment:" << path;
        return false;
    }

    json doc;
    try {
        const QByteArray bytes = file.readAll();
        doc = json::parse(bytes.constData(), bytes.constData() + bytes.size());
    }
    catch (const json::exception& e) {
        qWarning() << "Environment JSON parse error:" << e.what();
        return false;
    }

    env = {};
    env.version = doc.value("version", 1);

    if (doc.contains("terrain") && doc["terrain"].is_object()) {
        const auto& jt = doc["terrain"];
        env.terrain.enabled = jt.value("enabled", false);
        env.terrain.visible = jt.value("visible", true);
        env.terrain.path =
            resolvePathFromEnvironment(path, QString::fromStdString(jt.value("path", std::string())))
                .toStdString();
        env.terrain.texturePath =
            resolvePathFromEnvironment(path, QString::fromStdString(jt.value("texturePath", std::string())))
                .toStdString();
        env.terrain.width = jt.value("width", env.terrain.width);
        env.terrain.depth = jt.value("depth", env.terrain.depth);
        env.terrain.height = jt.value("height", env.terrain.height);
        if (jt.contains("offset"))
            env.terrain.offset = readVec3(jt["offset"]);
        env.terrain.meshCellsX = jt.value("meshCellsX", env.terrain.meshCellsX);
        env.terrain.meshCellsZ = jt.value("meshCellsZ", env.terrain.meshCellsZ);
        if (env.terrain.path.empty())
            env.terrain.enabled = false;
    }

    if (doc.contains("camera") && doc["camera"].is_object()) {
        const auto& jc = doc["camera"];
        if (jc.contains("target"))
            env.camera.target = readVec3(jc["target"]);
        env.camera.distance = jc.value("distance", env.camera.distance);
        env.camera.yaw = jc.value("yaw", env.camera.yaw);
        env.camera.pitch = jc.value("pitch", env.camera.pitch);
        env.camera.fovDeg = jc.value("fovDeg", env.camera.fovDeg);
    }

    return true;
}

bool EnvironmentSerializer::saveToFile(const QString& path, const EnvironmentState& env) {
    json doc;
    doc["version"] = 1;
    doc["terrain"] = {
        {"enabled", env.terrain.enabled},
        {"visible", env.terrain.visible},
        {"path", makePathRelativeToEnvironment(path, QString::fromStdString(env.terrain.path)).toStdString()},
        {"texturePath", makePathRelativeToEnvironment(path, QString::fromStdString(env.terrain.texturePath)).toStdString()},
        {"width", env.terrain.width},
        {"depth", env.terrain.depth},
        {"height", env.terrain.height},
        {"offset", writeVec3(env.terrain.offset)},
        {"meshCellsX", env.terrain.meshCellsX},
        {"meshCellsZ", env.terrain.meshCellsZ}
    };
    doc["camera"] = {
        {"target", writeVec3(env.camera.target)},
        {"distance", env.camera.distance},
        {"yaw", env.camera.yaw},
        {"pitch", env.camera.pitch},
        {"fovDeg", env.camera.fovDeg}
    };

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        qWarning() << "Cannot write environment:" << path;
        return false;
    }
    const QByteArray bytes = QByteArray::fromStdString(doc.dump(2));
    if (file.write(bytes) != bytes.size()) {
        qWarning() << "Cannot fully write environment:" << path;
        return false;
    }
    return true;
}
