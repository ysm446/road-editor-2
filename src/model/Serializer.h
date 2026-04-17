#pragma once

#include "RoadNetwork.h"
#include <QString>

class Serializer {
public:
    static bool loadFromFile(const QString& path, RoadNetwork& net);
    static bool saveToFile  (const QString& path, const RoadNetwork& net);
};
