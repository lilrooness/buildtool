//
// Created by jfran on 11/7/2026.
//

#include "ConfigurationGenerator.h"

#include <fstream>
#include <set>

#include "CMakePresetsWriter.h"
#include "VcpkgConfigurationMerger.h"

ConfigurationGenerator::ConfigurationGenerator(const std::string& lockfile_path) {
    std::ifstream lockfile(lockfile_path);
    json lockfileData = json::parse(lockfile);

    extra_directories_blob = lockfileData["extra_directories"];
    configuration_blob =  lockfileData["configuration"];
    manifest_blob =  lockfileData["manifest"];
    env_vars_blob = lockfileData["environment"];
    if (lockfileData.contains("overlay_triplets_path")) {
        overlayTripletsPath = lockfileData["overlay_triplets_path"];
    }
    if (lockfileData.contains("target_triplet")) {
        targetTripletName = lockfileData["target_triplet"];
    }
}

void ConfigurationGenerator::Generate(const std::string& user_project_root) {
    // generate directory structure from any directories & files encoded in lockfile
    // (this includes overlay triplet dirs)
    std::filesystem::path generated_configuration_dest(user_project_root);
    generated_configuration_dest.append(".generated_configuration");
    std::filesystem::create_directory(generated_configuration_dest);
    generate_directory_structure_from_json_blob(extra_directories_blob, generated_configuration_dest);

    // read user's vcpkg.json file
    std::filesystem::path user_project_vcpkg_manifest_path(user_project_root);
    user_project_vcpkg_manifest_path.append("vcpkg.json");
    std::ifstream user_project_vcpkg_manifest_file(user_project_vcpkg_manifest_path);
    json user_vcpkg_manifest_blob = json::parse(user_project_vcpkg_manifest_file);

    // read user's vcpkg-configuration.json file
    std::filesystem::path user_project_vcpkg_configuration_path(user_project_root);
    user_project_vcpkg_configuration_path.append("vcpkg-configuration.json");
    std::ifstream user_project_vcpkg_configuration_file(user_project_vcpkg_configuration_path);
    json user_vcpkg_configuration_blob = json::parse(user_project_vcpkg_configuration_file);

    manifest_blob = MergeManifests(user_vcpkg_manifest_blob, manifest_blob);
    configuration_blob = MergeConfigurations(user_vcpkg_configuration_blob, configuration_blob);

    // write generated vcpkg.json file
    std::filesystem::path vcpkg_manifest_file_path(generated_configuration_dest);
    vcpkg_manifest_file_path.append("vcpkg.json");
    std::ofstream vcpkg_manifest_file(vcpkg_manifest_file_path);
    vcpkg_manifest_file << manifest_blob.dump(2);

    // write generated vcpkg-configuration.json file
    std::filesystem::path vcpkg_configuration_file_path(generated_configuration_dest);
    vcpkg_configuration_file_path.append("vcpkg-configuration.json");
    std::ofstream vcpkg_configuration_file(vcpkg_configuration_file_path);
    vcpkg_configuration_file << configuration_blob.dump(2);

    CMakePreset preset("generated-preset");
    preset.data["environment"] = env_vars_blob;
    preset.AddCacheVariable("VCPKG_MANIFEST_DIR", "${sourceDir}/" + generated_configuration_dest.filename().string());
    if (!overlayTripletsPath.empty()) {
        std::filesystem::path overylayTripletsStdFSPath = overlayTripletsPath;
        std::filesystem::path overlayTripletsFullPath = generated_configuration_dest / overylayTripletsStdFSPath;
        preset.AddCacheVariable("VCPKG_OVERLAY_TRIPLETS", "${sourceDir}/" + overlayTripletsFullPath.string());
    }

    if (!targetTripletName.empty()) {
        preset.AddCacheVariable("VCPKG_TARGET_TRIPLET", targetTripletName);
    }

    if (!inheritsPreset.empty()) {
        preset.SetInherits(inheritsPreset);
    }

    CMakePresetsWriter writer(3, 3, 31, 0);
    writer.AddPreset(preset);
    std::filesystem::path cmakeUserPresetsLocation(user_project_root);
    cmakeUserPresetsLocation.append("CMakeUserPresets.json");
    writer.Write(cmakeUserPresetsLocation.string());
}

void ConfigurationGenerator::SetParentPresetName(const std::string &name) {
    inheritsPreset = name;
}

void ConfigurationGenerator::generate_directory_structure_from_json_blob(json blob, const std::filesystem::path& destination) {
    for (auto f : blob.items()) {
        if (f.value().is_object()) {
            auto childPath = destination;
            childPath.append(f.key());
            std::filesystem::create_directory(childPath);
            generate_directory_structure_from_json_blob(f.value(), childPath);
        }
        else if (f.value().is_string()) {
            auto fileDestination = destination;
            fileDestination.append(f.key());
            std::ofstream file(fileDestination);
            std::string contents = f.value();
            file << contents;
        }
    }
}
