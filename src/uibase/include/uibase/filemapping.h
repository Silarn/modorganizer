#pragma once
#include <QString>

#include <vector>

struct Mapping {
    QString source;
    QString destination;
    bool isDirectory;
    bool createTarget;
};

using MappingType = std::vector<Mapping>;
