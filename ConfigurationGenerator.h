//
// Created by jfran on 11/7/2026.
//

#ifndef BUILDTOOL_CONFIGURATIONGENERATOR_H
#define BUILDTOOL_CONFIGURATIONGENERATOR_H

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>
using nlohmann::json;

class ConfigurationGenerator {
public:
    ConfigurationGenerator(const std::string& lockfile_path);
    void Generate(const std::string& user_project_root);

private:
    json lockfileData;
    json extra_directories_blob;
    json configuration_blob;
    json manifest_blob;

    std::string normalize_git_repository_url(const std::string& url);
    void generate_directory_structure_from_json_blob(json blob, const std::filesystem::path& destination);
};


#endif //BUILDTOOL_CONFIGURATIONGENERATOR_H
