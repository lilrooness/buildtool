//
// Created by jfran on 11/7/2026.
//

#include "CMakePresetsWriter.h"
#include <fstream>
#include <set>

CMakePresetsWriter::CMakePresetsWriter(int ver, int maj, int min, int pat) {
    cmakePresetFileObject["version"] = ver;
    cmakePresetFileObject["cmakeMinimumRequired"] = {
        {"major", maj},
        {"minor", min},
        {"patch", pat}
    };
}

void CMakePresetsWriter::AddPreset(CMakePreset preset) {
    cmakePresetFileObject["configurePresets"].push_back(preset.data);
}

void CMakePresetsWriter::Write(std::string output_path) {
    std::filesystem::path output_std_filesystem_path(output_path);
    if (output_std_filesystem_path.is_relative()) {
        output_std_filesystem_path = std::filesystem::absolute(output_std_filesystem_path);
    }
    auto parentPath = output_std_filesystem_path.parent_path();

    if (is_directory(parentPath)) {
        for (auto child : std::filesystem::directory_iterator{parentPath}) {
            if (child.path().filename() == "CMakeUserPresets.json") {
                std::ifstream existingUserPresetsFile(child.path());

                auto existingPresets = json::parse(existingUserPresetsFile);

                if (existingPresets.contains("configurePresets")) {
                    auto existingConfigurationPresets = existingPresets["configurePresets"];
                    auto ourPresets = cmakePresetFileObject["configurePresets"];
                    auto mergedPresets = MergePresetsLists(existingConfigurationPresets, ourPresets);
                    cmakePresetFileObject["configurePresets"] = mergedPresets;
                }
            }
        }
    }
    std::ofstream ofile(output_path);
    ofile << cmakePresetFileObject.dump(2);
}

json::array_t CMakePresetsWriter::MergePresetsLists(json::array_t users, json::array_t ours) {
    std::set<std::string> presetNames;
    json::array_t presets;
    for (auto preset : ours) {
        std::string name = preset["name"];
        presetNames.insert(name);
        presets.push_back(preset);
    }

    for (auto preset : users) {
        if (!preset.contains("name")) {
            continue;
        }
        std::string name = preset["name"];
        if (!presetNames.contains(name)) {
            presetNames.insert(name);
            presets.push_back(preset);
        }
    }

    return presets;
}

CMakePreset::CMakePreset(const std::string& name) {
    data["name"] = name;
}

void CMakePreset::SetToolchainFile(const std::string& toolchainFilePath) {
    data["toolchainFile"] = toolchainFilePath;
}

void CMakePreset::SetBinaryDir(const std::string &binaryDir) {
    data["binaryDir"] = binaryDir;
}

void CMakePreset::SetHidden(bool hidden) {
    data["hidden"] = hidden;
}

void CMakePreset::SetInherits(const std::string &parentName) {
    data["inherits"] = parentName;
}

void CMakePreset::AddCacheVariable(const std::string &name, const std::string& value) {
    if (!data.contains("cacheVariables")) {
        data["cacheVariables"] = json::object();
    }

    data["cacheVariables"][name] = value;
}

template<typename T>
void CMakePreset::AddEnvironmentVariable(const std::string& name, T value) {
    if (!data.contains("environment")) {
        data["environment"] = json::object();
    }

    data["environment"][name] = value;
}
