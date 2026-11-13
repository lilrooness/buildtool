//
// Created by josephf on 12/11/2026.
//

#include "ProjectResolver.h"

#include <iostream>


std::filesystem::path ProjectResolver::Resolve() {
    if (m_locator.type == ProjectLocatorType::LOCAL) {
        return m_locator.localPath;
    }

    auto git_clone_path = std::filesystem::temp_directory_path();
    git_clone_path.append(".temporary_clone_dir");
    std::filesystem::create_directory(git_clone_path);

    std::stringstream git_command_stream;
    git_command_stream << "git clone " << m_locator.git_url << " " << git_clone_path;
    std::string command = git_command_stream.str();
    std::cout << "running '" << command << "' ..." << std::endl;
    auto ret = std::system(command.c_str());
    if (ret == 0 && errno == ENOENT || ret == -1) {
        std::cerr << "Error cloning git url: " << m_locator.git_url << std::endl;
        exit(-1);
    }
    git_command_stream.str("");
    std::cout << "done." << std::endl;

    const auto saved_working_dir = std::filesystem::current_path();
    std::filesystem::current_path(git_clone_path.string());
    {
        git_command_stream << "git fetch --tags";
        command = git_command_stream.str();
        std::cout << "running '" << command << std::endl;
        ret = std::system(command.c_str());
        if (ret == 0 && errno == ENOENT || ret == -1) {
            std::cerr << "Error fetching tags" << std::endl;
            exit(-1);
        }
        git_command_stream.str("");

        git_command_stream << "git checkout " << m_locator.git_ref;
        command = git_command_stream.str();
        std::cout << "Running " << command << std::endl;
        ret = std::system(command.c_str());
        if (ret == 0 && errno == ENOENT || ret == -1) {
            std::cerr << "Error checking out git ref: " << m_locator.git_ref << std::endl;
            std::filesystem::path(saved_working_dir.string());
            std::filesystem::remove(git_clone_path);
            exit(-1);
        }
        git_command_stream.str("");
    }
    std::filesystem::current_path(saved_working_dir.string());

    m_path_to_cloned_repo = git_clone_path;

    return git_clone_path;
}

void ProjectResolver::Clean() {
    if (m_locator.type == ProjectLocatorType::LOCAL) {
        return;
    }

    std::uintmax_t code = std::filesystem::remove_all(m_path_to_cloned_repo);
    if (code == -1) {
        std::cout << "Could not delete " << m_path_to_cloned_repo << std::endl;
        std::cout << "Error Code " << code << std::endl;
    }

}
