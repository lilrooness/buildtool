//
// Created by josephf on 12/11/2026.
//

#ifndef BUILDTOOL_PROJECTRESOLVER_H
#define BUILDTOOL_PROJECTRESOLVER_H

#include <string>
#include <filesystem>

enum class ProjectLocatorType {
    GIT, LOCAL
};

struct ProjectLocator{
    std::string git_url;
    std::string git_ref;
    std::string localPath;
    ProjectLocatorType type;
};

class ProjectResolver {
public:
    ProjectResolver(const ProjectLocator &locator) : m_locator(locator) {}
    std::filesystem::path Resolve();
    void Clean();
    ProjectLocator m_locator;

private:
    std::filesystem::path m_path_to_cloned_repo;
};


#endif //BUILDTOOL_PROJECTRESOLVER_H
