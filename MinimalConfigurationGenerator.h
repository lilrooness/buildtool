//
// Created by josephf on 12/11/2026.
//

#ifndef BUILDTOOL_MINIMALCONFIGURATIONGENERATOR_H
#define BUILDTOOL_MINIMALCONFIGURATIONGENERATOR_H

#include <filesystem>
#include <string>

#include "Dependency.h"
#include "ProjectResolver.h"

/*
 * MinimalConfigurationGenerator is like ConfigurationGenerator in that it produces only a vcpkg.json file
 * and a vcpkg-configuration.json file. It does so by reading .spdx files, vcpkg.json file and a vcpkg.configuration
 * file for the build tree you want to target. Unlike ConfigurationGenerator, it does not require a full
 * cmake build folder.
 *
 * This is useful if you only have access to .spdx files and vcpkg configuration for a vcpkg build tree, and not the
 * original build folder (if for instance your build took place on another machine/CI)
 *
 * It takes an SPDX files root, from which it searches recursively for spdx files, reading all that it finds.
 * It also takes a ProjectLocator, which can reference either a Git url + ref, or a local file path to a folder containing
 * a vcpkg.json file and a vcpkg-configuration.json file that will be used to generate a new
 * vcpkg-configration file & a new vcpkg.json file
 */
class MinimalConfigurationGenerator {
public:
    std::vector<Dependency> m_dependencies;
    std::map<std::string, std::vector<std::string>> m_registry_to_packages;

    void GenerateConfigurationForLocalProject(
        std::filesystem::path& path_to_project_to_configure,
        const ProjectLocator& project_locator
    );
    void AddSPDXRoot(const std::filesystem::path& spdx_files_root);
};


#endif //BUILDTOOL_MINIMALCONFIGURATIONGENERATOR_H
