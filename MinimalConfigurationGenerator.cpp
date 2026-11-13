//
// Created by josephf on 12/11/2026.
//

#include <fstream>
#include <map>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>

#include "MinimalConfigurationGenerator.h"
#include "VcpkgConfigurationMerger.h"

using nlohmann::json;

void MinimalConfigurationGenerator::AddSPDXRoot(const std::filesystem::path& spdx_files_root) {
    if (!std::filesystem::is_directory(spdx_files_root)) {
        std::cerr << "no such directory: " << spdx_files_root << std::endl;
        exit(1);
    }
    // traverse deps in share folder
    for (auto childDir : std::filesystem::directory_iterator(spdx_files_root)) {
        auto childPath = childDir.path();
        childPath.append("vcpkg.spdx.json");

        if (std::filesystem::exists(childPath)) {
            std::cout << "parsing spdx file: " << childPath << std::endl;
            std::ifstream f(childPath);
            json data = json::parse(f);

            std::string name = data["packages"][0]["name"];
            std::string version = data["packages"][0]["versionInfo"];
            std::string registry = data["packages"][0]["downloadLocation"];

            auto location = registry.find("git+");
            if (location == std::string::npos) {
                std::cout << "WARNING: " << name << " did not come from a vcpkg git registry, skipping ..." << std::endl;
                continue;
            }
            auto lastPart = registry.substr(location+4, std::string::npos);
            location = lastPart.find(".git@");
            if (location == std::string::npos) {
                std::cout << "WARNING: " << name << " has an unsupported registry downloadLocation url " << registry << " skipping ..." << std::endl;
                continue;
            }

            auto urlOnly = lastPart.substr(0, location+4);
            m_registry_to_packages[urlOnly].push_back(name);

            m_dependencies.push_back({name, version, urlOnly});
        }
    }
}

void MinimalConfigurationGenerator::GenerateConfigurationForLocalProject(std::filesystem::path& path_to_project_to_configure, const ProjectLocator &project_locator) {
    json generated_manifest_object = json::object();

    if (!std::filesystem::is_directory(path_to_project_to_configure)) {
        std::cerr << "no such directory: " << path_to_project_to_configure << std::endl;
        exit(1);
    }

    std::cout << "Attempting to resolve target project ..." << std::endl;
    ProjectResolver resolver(project_locator);
    auto path_to_project_to_match = resolver.Resolve();
    std::cout << "Target project resolved to: " << path_to_project_to_match << std::endl;

    auto path_to_vcpkg_manifest_file = path_to_project_to_match;
    path_to_vcpkg_manifest_file.append("vcpkg.json");
    auto path_to_vcpkg_configuration_file = path_to_project_to_match;
    path_to_vcpkg_configuration_file.append("vcpkg-configuration.json");

    if (!std::filesystem::is_regular_file(path_to_vcpkg_manifest_file)) {
        std::cerr << "Could not find vcpkg.json file " << path_to_vcpkg_manifest_file << std::endl;
        resolver.Clean();
        exit(1);
    }
    if (!std::filesystem::is_regular_file(path_to_vcpkg_configuration_file)) {
        std::cerr << "Could not find vcpkg-configuration.json file " << path_to_vcpkg_configuration_file << std::endl;
        resolver.Clean();
        exit(1);
    }

    std::ifstream target_manifest_file(path_to_vcpkg_manifest_file);
    std::ifstream target_configuration_file(path_to_vcpkg_configuration_file);

    json target_manifest_blob = json::parse(target_manifest_file);
    json target_configuration_blob = json::parse(target_configuration_file);
    target_manifest_file.close();
    target_configuration_file.close();
    resolver.Clean();

    auto path_to_user_vcpkg_manifest = path_to_project_to_configure;
    auto path_to_user_vcpkg_configuration = path_to_project_to_configure;
    path_to_user_vcpkg_manifest.append("vcpkg.json");
    path_to_user_vcpkg_configuration.append("vcpkg-configuration.json");

    if (!std::filesystem::is_regular_file(path_to_user_vcpkg_manifest)) {
        std::cerr << "Could not find vcpkg.json file: " << path_to_user_vcpkg_manifest << std::endl;
        exit(1);
    }

    if (!std::filesystem::is_regular_file(path_to_user_vcpkg_configuration)) {
        std::cerr << "Could not find vcpkg-configuration.json file: " << path_to_user_vcpkg_configuration << std::endl;
        exit(1);
    }

    json user_manifest_blob = json::parse(std::ifstream(path_to_user_vcpkg_manifest));
    json user_configuration_blob = json::parse(std::ifstream(path_to_user_vcpkg_configuration));

    json output_manifest = MergeManifests(user_manifest_blob, target_manifest_blob, m_dependencies);
    json output_configuration = MergeConfigurations(user_configuration_blob, target_configuration_blob);

    std::ofstream output_manifest_file (path_to_user_vcpkg_manifest);
    output_manifest_file << output_manifest.dump(2);

    std::ofstream output_configuration_file (path_to_user_vcpkg_configuration);
    output_configuration_file << output_configuration.dump(2);
}
