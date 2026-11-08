//
// Created by jfran on 11/7/2026.
//

#ifndef BUILDTOOL_PRESETSWRITER_H
#define BUILDTOOL_PRESETSWRITER_H

#include <string>
#include <nlohmann/json.hpp>
using nlohmann::json;

const static json CMakePresetBase = json::object({
    {"version", 3},
    {"cmakeMinimumRequired", {
        {"major", 3},
        {"minor", 31},
        {"patch", 0}
      }},
      {"configurePresets", json::array()}
});

class CMakePreset {
public:
    CMakePreset(const std::string& name);
    void SetToolchainFile(const std::string& toolchainFilePath);
    void SetBinaryDir(const std::string &binaryDir);
    void SetHidden(bool hidden);
    void SetInherits(const std::string &parentName);
    void AddCacheVariable(const std::string &name, const std::string& value);
    template<typename T>
    void AddEnvironmentVariable(const std::string& name, T value);
    json data;
};

class CMakePresetsWriter {
public:
    CMakePresetsWriter(int ver=3, int maj=3, int min=31, int pat=0);
    void AddPreset(CMakePreset preset);
    void Write(std::string output_path);
    json::array_t MergePresetsLists(json::array_t users, json::array_t ours);
private:
    json cmakePresetFileObject = CMakePresetBase;
};


#endif //BUILDTOOL_PRESETSWRITER_H
