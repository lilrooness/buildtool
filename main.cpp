#include <iostream>
#include <argparse/argparse.hpp>
#include <string>
#include <vector>

#include "ConfigurationGenerator.h"
#include "Locker.h"

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

    std::vector<std::string> env_vars;
    lockCommand.add_argument("--env", "-e")
    .append()
    .nargs(2)
    .store_into(env_vars)
    .help("name and value of an environment variable needed for the build <NAME> <VALUE>");

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

    std::string generatedPresetParentName;
    generateCommand.add_argument("-i")
        .default_value("")
        .store_into(generatedPresetParentName)
        .help("If set, the generated preset 'inherits' field will be set to this");

    try {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    if (!cmake_build_directory_path_string.empty()) {
        Locker locker(cmake_build_directory_path_string, copy_directories, env_vars);
        return locker.Lock(lockfile_out_path);
    }

    if (!lockfile_path.empty()) {
        ConfigurationGenerator generator(lockfile_path);
        if (!generatedPresetParentName.empty()) {
            generator.SetParentPresetName(generatedPresetParentName);
        }
        generator.Generate(user_project_root);
    }

    return 0;
}
