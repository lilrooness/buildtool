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
    void SetParentPresetName(const std::string &);

private:
    json lockfileData;
    json extra_directories_blob;
    json configuration_blob;
    json manifest_blob;
    json env_vars_blob;
    std::string overlayTripletsPath;
    std::string targetTripletName;
    std::string inheritsPreset;

    void generate_directory_structure_from_json_blob(json blob, const std::filesystem::path& destination);
};


#endif //BUILDTOOL_CONFIGURATIONGENERATOR_H
