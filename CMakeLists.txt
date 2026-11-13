cmake_minimum_required(VERSION 4.3)
project(buildtool)

find_package(argparse CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 20)

add_executable(buildtool main.cpp
        CMakeCacheParser.cpp
        CMakeCacheParser.h
        ConfigurationGenerator.cpp
        ConfigurationGenerator.h
        Locker.cpp
        Locker.h
        Dependency.h
        CMakePresetsWriter.cpp
        CMakePresetsWriter.h
        VcpkgConfigurationMerger.cpp
        VcpkgConfigurationMerger.h
        ProjectResolver.cpp
        ProjectResolver.h
        MinimalConfigurationGenerator.cpp
        MinimalConfigurationGenerator.h
        )

target_link_libraries(buildtool PRIVATE argparse::argparse nlohmann_json::nlohmann_json)