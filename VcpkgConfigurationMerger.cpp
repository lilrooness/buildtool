//
// Created by josephf on 13/11/2026.
//

#include <set>
#include <iostream>

#include "VcpkgConfigurationMerger.h"
#include "Dependency.h"

std::string normalize_git_repository_url(const std::string& url);

json MergeConfigurations(json user_configuration_blob, json target_configuration_blob) {

    // merge the configured registries lists between the locked project and the user's project
    json user_configuration_registries_list = user_configuration_blob["registries"];
    std::map<std::string, std::string> normalized_url_to_registry_url;
    std::map<std::string, std::set<std::string> > normalized_registry_url_to_packages_set;
    std::map<std::string, std::string> normalized_registry_url_to_baseline;

    for (json registry : target_configuration_blob["registries"]) {
        std::string repository_url = registry["repository"];
        auto normalized_url = normalize_git_repository_url(repository_url);
        normalized_url_to_registry_url[normalized_url] = repository_url;
        normalized_registry_url_to_baseline[normalized_url] = registry["baseline"];
        json packages = registry["packages"];
        std::copy(
            packages.begin(), packages.end(),
            std::inserter(
                normalized_registry_url_to_packages_set[normalized_url],
                normalized_registry_url_to_packages_set[normalized_url].end()
            )
        );
    }

    for (json registry : user_configuration_blob["registries"]) {
        std::string repository_url = registry["repository"];
        auto normalized_url = normalize_git_repository_url(repository_url);
        if (!normalized_url_to_registry_url.contains(normalized_url)) {
            normalized_url_to_registry_url[normalized_url] = repository_url;
            normalized_registry_url_to_baseline[normalized_url] = registry["baseline"];
        }
        json packages = registry["packages"];
        std::copy(
            packages.begin(), packages.end(),
            std::inserter(
                normalized_registry_url_to_packages_set[normalized_url],
                normalized_registry_url_to_packages_set[normalized_url].end()
            )
        );
    }

    json new_registries_list = json::array();
    for (auto pair : normalized_registry_url_to_packages_set) {
        json thisRegitry = json::object();
        thisRegitry["kind"] = "git";
        thisRegitry["baseline"] = normalized_registry_url_to_baseline[pair.first];
        thisRegitry["repository"] = normalized_url_to_registry_url[pair.first];

        json packages = json::array();
        for (auto package : pair.second) {
            packages.push_back(package);
        }
        thisRegitry["packages"] = packages;
        new_registries_list.push_back(thisRegitry);
    }

    json output_configuration_blob = target_configuration_blob;
    output_configuration_blob["registries"] = new_registries_list;

    return output_configuration_blob;
}

json MergeManifests(json user_manifest_blob, json target_manifest_blob, std::vector<Dependency> exiting_overrides/*={}*/) {
    std::set<std::string> user_project_overrides;
    std::set<std::string> matcher_project_overrides;
    std::vector<Dependency> user_project_override_dependencies;
    std::vector<Dependency> target_project_override_dependencies;
    std::set<std::string> shared_overrides;

    if (user_manifest_blob.contains("overrides")) {
        for (auto override : user_manifest_blob["overrides"]) {
            std::string name = override["name"];
            std::string version = override["version"];
            user_project_overrides.insert(name);
            Dependency d(name, version);
            user_project_override_dependencies.push_back(d);
        }
    }

    for (Dependency& override : exiting_overrides) {
        matcher_project_overrides.insert(override.name);
        target_project_override_dependencies.push_back(override);
    }

    if (target_manifest_blob.contains("overrides")) {
        for (auto override : target_manifest_blob["overrides"]) {
            std::string name = override["name"];
            std::string version = override["version"];
            if (!matcher_project_overrides.contains(name)) {
                matcher_project_overrides.insert(name);
                Dependency d(name, version);
                target_project_override_dependencies.push_back(d);
            }
        }
    }


    std::ranges::set_intersection(
        user_project_overrides, matcher_project_overrides,
        std::inserter(shared_overrides, shared_overrides.begin())
    );

    // if the user and the build tree to match share override dependencies, use the version in the matching build tree
    json output_overrides_json_list = json::array();
    for (auto dep : user_project_override_dependencies) {
        std::cout << "Target override preferred over user's for: " << dep.name << std::endl;
        if (!shared_overrides.contains(dep.name)) {
            output_overrides_json_list.push_back({
                {"name", dep.name},
                {"version", dep.version},
            });
        }
    }

    for (auto dep : target_project_override_dependencies) {
        output_overrides_json_list.push_back({
            {"name", dep.name},
            {"version", dep.version},
        });
    }

    json output_manifest = user_manifest_blob;
    output_manifest["overrides"] = output_overrides_json_list;

    return output_manifest;
}

std::string normalize_git_repository_url(const std::string& url) {
    std::string url_without_protocol;
    if (url.starts_with("git@")) {
        auto ampersand_pos = url.find('@');
        url_without_protocol = url.substr(ampersand_pos+1);
        auto colon_pos = url_without_protocol.find(':');
        url_without_protocol[colon_pos] = '/';
    }
    else if (url.starts_with("https://")) {
        auto doubleSlashPos = url.find("//");
        url_without_protocol = url.substr(doubleSlashPos+2);
    }
    else {
        return url;
    }

    if (!url_without_protocol.ends_with(".git")) {
        url_without_protocol += ".git";
    }
    return url_without_protocol;
}