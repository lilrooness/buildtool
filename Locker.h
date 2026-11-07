//
// Created by jfran on 11/7/2026.
//

#ifndef BUILDTOOL_LOCKER_H
#define BUILDTOOL_LOCKER_H

#include <string>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>

using nlohmann::json;

class Locker {
public:
    Locker(const std::string cmake_build_directory_path_string, std::vector<std::string> copy_directories) :
    cmake_build_directory_path_string(cmake_build_directory_path_string),
    copy_directories(copy_directories) {};

    int Lock(const std::string& lockfile_out_path);
private:

    json get_directory_as_json_blob(std::filesystem::path& path);

    std::string cmake_build_directory_path_string;
    std::vector<std::string> copy_directories;
};


#endif //BUILDTOOL_LOCKER_H
