//
// Created by jfran on 11/7/2026.
//

#ifndef BUILDTOOL_DEPENDENCY_H
#define BUILDTOOL_DEPENDENCY_H

#include <string>

struct Dependency {
    Dependency(std::string name, std::string version, std::string registry) :
        name(std::move(name)),
        version(std::move(version)),
        registry(std::move(registry)) {};

    std::string name;
    std::string version;
    std::string registry;
};


#endif //BUILDTOOL_DEPENDENCY_H

