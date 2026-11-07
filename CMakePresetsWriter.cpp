//
// Created by jfran on 11/7/2026.
//

#include "CMakePresetsWriter.h"
#include <fstream>

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
    std::ofstream ofile(output_path);
    ofile << cmakePresetFileObject.dump(2);
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
