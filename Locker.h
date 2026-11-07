//
// Created by jfran on 11/7/2026.
//

#ifndef BUILDTOOL_LOCKER_H
#define BUILDTOOL_LOCKER_H

#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>

using nlohmann::json;

class Locker {
public:
    Locker(std::string  cmake_build_directory_path_string, const std::vector<std::string>& copy_directories, const std::vector<std::string>& env_vars) :
    cmake_build_directory_path_string(std::move(cmake_build_directory_path_string)),
    copy_directories(copy_directories),
    env_vars(env_vars){};

    int Lock(const std::string& lockfile_out_path);
private:

    json get_directory_as_json_blob(std::filesystem::path& path);

    std::string cmake_build_directory_path_string;
    std::vector<std::string> copy_directories;
    std::vector<std::string> env_vars;
};


#endif //BUILDTOOL_LOCKER_H
