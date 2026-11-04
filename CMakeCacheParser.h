//
// Created by jfran on 11/1/2026.
//

#ifndef BUILDTOOL_CMAKECACHEPARSER_H
#define BUILDTOOL_CMAKECACHEPARSER_H

#include <map>
#include <fstream>
#include <string>


struct CMakeCacheParser {
    CMakeCacheParser(std::ifstream& file);
    std::map<std::string, std::string> variables;
};


#endif //BUILDTOOL_CMAKECACHEPARSER_H
