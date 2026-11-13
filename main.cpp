#include <iostream>
#include <argparse/argparse.hpp>
#include <string>
#include <vector>

#include "ConfigurationGenerator.h"
#include "Locker.h"
#include "MinimalConfigurationGenerator.h"
#include "ProjectResolver.h"

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

    // Target Mode
    std::cout << "Constructing target command" << std::endl;
    argparse::ArgumentParser targetCommand("target");
    program.add_subparser(targetCommand);

    std::string user_project_path;
    targetCommand.add_argument("path")
        .store_into(user_project_path)
        .default_value(".")
        .help("Path to the root of the project you wish to configure to the target configuration");

    std::string target_spdx_path;
    targetCommand.add_argument("--spdx", "-x")
        .store_into(target_spdx_path)
        .help("Path to the root directory containing SPDX files to produce version pins. Optional");

    auto& group = targetCommand.add_mutually_exclusive_group(true);
    std::string local_target_path;
    group.add_argument("--local_target", "-l")
        .store_into(local_target_path)
        .help("Path to a local project you would like to target your dependency tree to EG -l /path/to/target/project");

    std::vector<std::string> git_target_url_and_ref;
    group.add_argument("--git_target", "-g")
        .nargs(2)
        .store_into(git_target_url_and_ref)
        .help("Git URL and ref of project you would like to target your dependency tree to: EG -g https://github.com/carbonengine/scheduler.git refs/tags/v1.6.0");

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

    if (program.is_subcommand_used("target")) {
        ProjectLocator project_locator;
        if (!git_target_url_and_ref.empty()) {
            project_locator.type = ProjectLocatorType::GIT;
            project_locator.git_url = git_target_url_and_ref[0];
            project_locator.git_ref = git_target_url_and_ref[1];
        }
        else if (!local_target_path.empty()) {
            project_locator.type = ProjectLocatorType::LOCAL;
            project_locator.localPath = local_target_path;
        }
        MinimalConfigurationGenerator minimal_configuration_generator;
        if (targetCommand.is_used("--spdx")) {
        minimal_configuration_generator.AddSPDXRoot(target_spdx_path);
        }


        std::filesystem::path user_project_stdfs_path(user_project_path);
        minimal_configuration_generator.GenerateConfigurationForLocalProject(user_project_stdfs_path, project_locator);
    }

    return 0;
}
