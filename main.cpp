#include <iostream>

#include <nlohmann/json.hpp>
#include <argparse/argparse.hpp>
#include <string>
#include <fstream>
#include <utility>
#include <vector>
#include <sstream>

#include "CMakeCacheParser.h"

using nlohmann::json;

struct Dependency {
    Dependency(std::string name, std::string version, std::string registry) :
        name(std::move(name)),
        version(std::move(version)),
        registry(std::move(registry)) {};

    std::string name;
    std::string version;
    std::string registry;
};

json get_directory_as_json_blob(std::filesystem::path& path) {
    if (!std::filesystem::is_directory(path)) {
        return json::object();
    }

    json blob = json::object();

    for (auto child : std::filesystem::directory_iterator(path)) {
        auto childPath = child.path();
        if (std::filesystem::is_directory(childPath)) {
            blob[childPath.filename().string()] = get_directory_as_json_blob(childPath);
        }
        else {
            std::ifstream file(childPath);
            std::stringstream ss;
            bool first = true;
            for (std::string line; std::getline(file, line);) {
                if (!first) {
                    ss << std::endl;
                }
                first = false;
                ss << line;
            }
            std::string entireFileString = ss.str();
            blob[childPath.filename().string()] = entireFileString;
        }
    }

    return blob;
}

void generate_directory_structure_from_json_blob(json blob, std::filesystem::path destination) {
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

int main(int argc, char** argv) {

    argparse::ArgumentParser program("buildtool");

    // Lock Mode
    argparse::ArgumentParser lockCommand("lock");
    program.add_subparser(lockCommand);

    std::string cmake_build_directory_path_string;
    lockCommand.add_argument("path")
        .store_into(cmake_build_directory_path_string)
        .help("the path to the cmake build directory you want to produce the lockfile for");

    std::string lockfile_out_path;
    lockCommand.add_argument("-o")
        .default_value("vcpkg_overrides_template.json")
        .store_into(lockfile_out_path)
        .help("output filename");

    std::vector<std::string> copy_directories;
    lockCommand.add_argument("--copy-directory", "-c")
        .append()
        .store_into(copy_directories)
        .help("Specify extra directories to recursively encoded (any overlay triplet directories will be automatically recursively encoded)");


    // Generate Mode
    argparse::ArgumentParser generateCommand("generate");
    program.add_subparser(generateCommand);

    std::string lockfile_path;
    generateCommand.add_argument("path")
        .store_into(lockfile_path)
        .help("The path to the lockfile you want to use to generate your configuration");

    std::string user_project_root;
    generateCommand.add_argument("-p")
        .store_into(user_project_root)
        .default_value(".")
        .help("Path to the root of the cmake project you want to generate a configuration for");

    std::vector<std::string> local_dependencies;
    generateCommand.add_argument("--local_dep", "-l")
        .nargs(2)
        .append()
        .store_into(local_dependencies)
        .help("Specify a local dependency '<DEPENDENCY NAME> <FILE PATH>'");

    std::vector<std::string> git_dependencies;
    generateCommand.add_argument("--git_dep", "-g")
        .nargs(2)
        .append()
        .store_into(git_dependencies)
        .help("Specify a dependency on github '<DEPENDENCY NAME> <GIT URL>'");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    if (!cmake_build_directory_path_string.empty()) {

        // read cmake cache file into a CMakeCacheParser object
        auto cmakeCacheFilePath = std::filesystem::path(cmake_build_directory_path_string);
        cmakeCacheFilePath.append("CMakeCache.txt");
        if(!std::filesystem::is_regular_file(cmakeCacheFilePath)) {
            std::cerr << "Cannot find CMakeCache file at: " << cmakeCacheFilePath << std::endl;
            return 1;
        }
        std::ifstream cmakeCacheFileIn(cmakeCacheFilePath);
        CMakeCacheParser cmakeCacheObj(cmakeCacheFileIn);

        auto manifestMode = cmakeCacheObj.variables["VCPKG_MANIFEST_MODE"];
        std::filesystem::path vcpkg_installed_dir = cmakeCacheObj.variables["_VCPKG_INSTALLED_DIR"];
        std::string triplet_name = cmakeCacheObj.variables["VCPKG_TARGET_TRIPLET"];
        std::filesystem::path vcpkg_root_dir = cmakeCacheObj.variables["Z_VCPKG_ROOT_DIR"];

        if (cmakeCacheObj.variables.contains("VCPKG_OVERLAY_PORTS")) {
            std::cerr << "Cannot create a lockfile for a build using overlay ports" << std::endl;
            return 1;
        }

        if (manifestMode != "ON") {
            std::cerr << "this tool only works for builds generated with vcpkg manifest mode" << std::endl;
            return 1;
        }

        if (cmakeCacheObj.variables.contains("VCPKG_OVERLAY_TRIPLETS")) {
            copy_directories.push_back(cmakeCacheObj.variables["VCPKG_OVERLAY_TRIPLETS"]);
        }

        // recursively encode the directory structure & all the contained files into a json object
        json encoded_directories_blob = json::object();
        if (copy_directories.size() > 1) {
            auto firstCopyDir = std::filesystem::path(copy_directories[0]);
            auto sharedRoot = firstCopyDir.root_name();
            size_t shortestDistanceFromRoot {std::numeric_limits<size_t>::max()};
            for (int i=1; i<copy_directories.size(); i++) {
                auto thisCopyDir = std::filesystem::path(copy_directories[i]);
                auto [thisDirItr, firstDirItr] =
                    std::mismatch(thisCopyDir.begin(), thisCopyDir.end(),
                                firstCopyDir.begin(), firstCopyDir.end());
                size_t thisDistance = std::distance(thisCopyDir.begin(), thisDirItr);
                if (thisDistance < shortestDistanceFromRoot) {
                    shortestDistanceFromRoot = thisDistance;
                }
            }
            auto current = firstCopyDir.begin();
            auto endItr = firstCopyDir.begin();
            std::advance(endItr, shortestDistanceFromRoot);
            std::filesystem::path closestCommonRoot;
            while (current != endItr) {
                closestCommonRoot.append(current->string());
                current++;
            }
            for (std::filesystem::path copyPath : copy_directories) {
                if (std::filesystem::is_directory(copyPath)) {
                    auto proximalPath  = std::filesystem::proximate(copyPath, closestCommonRoot);
                    json* node = &encoded_directories_blob;
                    for (auto& el : proximalPath) {
                        if (el == proximalPath.filename()) {
                            (*node)[el.string()] = get_directory_as_json_blob(copyPath);
                        }
                        else if (!node->contains(el.string())) {
                            (*node)[el.string()] = json::object();
                            node = &node->at(el.string());
                        }
                        else {
                            node = &node->at(el.string());
                        }
                    }
                }
            }
        }
        else {
            std::filesystem::path overlayTripletsPath = copy_directories[0];
            encoded_directories_blob[overlayTripletsPath.filename().string()] = get_directory_as_json_blob(overlayTripletsPath);
        }

        std::filesystem::path vcpkg_installed_triplet_dir(vcpkg_installed_dir);

        vcpkg_installed_triplet_dir.append(triplet_name);

        std::filesystem::path vcpkg_triplet_share_dir(vcpkg_installed_triplet_dir);
        vcpkg_triplet_share_dir.append("share");

        std::vector<Dependency> dependencies;
        std::map<std::string, std::vector<std::string>> registry_to_packages;

        // traverse deps in share folder
        for (auto childDir : std::filesystem::directory_iterator(vcpkg_triplet_share_dir)) {
            auto childPath = childDir.path();
            childPath.append("vcpkg.spdx.json");

            if (std::filesystem::exists(childPath)) {
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
                registry_to_packages[urlOnly].push_back(name);

                dependencies.push_back({name, version, urlOnly});
            }
        }

        std::filesystem::path vcpkg_manifest_path = cmakeCacheObj.variables["VCPKG_MANIFEST_DIR"];
        std::filesystem::path vcpkg_config_path = vcpkg_manifest_path;
        vcpkg_config_path.append("vcpkg-configuration.json");

        std::ifstream vcpkg_configuration_file(vcpkg_config_path);
        json vcpkg_configuration_data = json::parse(vcpkg_configuration_file);

        std::map<std::string, std::string> package_name_to_registry_baseline;

        if (vcpkg_configuration_data.contains("registries")) {
            for (auto registry_blob : vcpkg_configuration_data["registries"]) {
                if (registry_blob.contains("packages")) {
                    for (std::string package : registry_blob["packages"]) {
                        package_name_to_registry_baseline[package] = registry_blob["baseline"];
                    }
                }
            }
        }

        std::string default_registry_kind = vcpkg_configuration_data["default-registry"]["kind"];
        std::string default_registry_baseline = vcpkg_configuration_data["default-registry"]["baseline"];
        std::string default_registry_repository = vcpkg_configuration_data["default-registry"]["repository"];

        json lockObject = {
            {"manifest", {
                        {"dependencies", json::array()},
                        {"overrides", json::array()}
            }},
        {"configuration", {
            {"default-registry", {
                {"kind", default_registry_kind},
                {"baseline", default_registry_baseline},
                {"repository", default_registry_repository}
            }},
            {"registries", json::array()}
            }},
            {"extra_directories", encoded_directories_blob}
        };

        for (auto entry : registry_to_packages) {
            auto registry = entry.first;
            auto package_names = entry.second;

            std::string packages_baseline;

            // all the packages that come from one registry, share a baseline.
            if (package_name_to_registry_baseline.contains(package_names[0])) {
                packages_baseline = package_name_to_registry_baseline[package_names[0]];
            }
            else {
                //this is the default registry.. skipping
                continue;
            }

            json registry_block = {
            {"kind", "git"},
            {"repository", registry},
            {"baseline", packages_baseline},
            {"packages", json::array()}
            };
            registry_block["packages"] = package_names;
            lockObject["configuration"]["registries"].push_back(registry_block);
        }

        for (auto d : dependencies) {
            std::cout << d.name << " " << d.version << std::endl;
            json override = {
                {"name", d.name},
                {"version", d.version}
            };
            lockObject["manifest"]["overrides"].push_back(override);
        }

        std::ofstream ofile(lockfile_out_path);
        ofile << lockObject.dump(2) << std::endl;
    }

    if (!lockfile_path.empty()) {
        std::ifstream lockfile(lockfile_path);
        json lockfileData = json::parse(lockfile);

        json extra_directories = lockfileData["extra_directories"];
        json configuration =  lockfileData["configuration"];
        json manifest =  lockfileData["manifest"];

        std::filesystem::path generated_configuration_dest(user_project_root);
        generated_configuration_dest.append(".generated_configuration");
        std::filesystem::create_directory(generated_configuration_dest);
        generate_directory_structure_from_json_blob(extra_directories, generated_configuration_dest);

        // TODO: read in the user's project vcpkg.json file to populate the dependencies array in the new manifest file
        std::filesystem::path vcpkg_manifest_file_path(generated_configuration_dest);
        vcpkg_manifest_file_path.append("vcpkg.json");
        std::ofstream vcpkg_manifest_file(vcpkg_manifest_file_path);
        vcpkg_manifest_file << manifest.dump(2);

        std::filesystem::path vcpkg_configuration_file_path(generated_configuration_dest);
        vcpkg_configuration_file_path.append("vcpkg-configuration.json");
        std::ofstream vcpkg_configuration_file(vcpkg_configuration_file_path);
        vcpkg_configuration_file << configuration.dump(2);
    }

    return 0;
}
