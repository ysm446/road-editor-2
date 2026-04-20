#pragma once

#include "EnvironmentState.h"
#include <QString>

class EnvironmentSerializer {
public:
    static bool loadFromFile(const QString& path, EnvironmentState& env);
    static bool saveToFile(const QString& path, const EnvironmentState& env);
};
