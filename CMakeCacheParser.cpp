//
// Created by jfran on 11/1/2026.
//

#include "CMakeCacheParser.h"

#include <iostream>

CMakeCacheParser::CMakeCacheParser(std::ifstream& file) {
    for (std::string line; std::getline(file, line); ) {
        if (line.empty()) {
            continue;
        }

        if (line[0] == '#' || line[0] == '/') {
            continue;
        }

        auto position = line.find('=');
        if (position == std::string::npos) {
            continue;
        }

        std::string nameAndType = line.substr(0, position);
        std::string value = line.substr(position+1);

        if (value.empty()) {
            continue;
        }

        auto typeSeperatorPos = nameAndType.find(':');
        if (typeSeperatorPos == std::string::npos) {
            // something weird here, there really should be a type seperator
            continue;
        }

        std::string name = nameAndType.substr(0, typeSeperatorPos);

        variables[name] = value;
    }
}
