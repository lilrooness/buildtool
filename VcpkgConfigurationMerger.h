//
// Created by josephf on 13/11/2026.
//

#ifndef BUILDTOOL_VCPKGCONFIGURATIONMERGER_H
#define BUILDTOOL_VCPKGCONFIGURATIONMERGER_H

#include <nlohmann/json.hpp>

#include "Dependency.h"

using nlohmann::json;

json MergeManifests(json user_manifest_blob, json target_manifest_blob, std::vector<Dependency> exiting_overrides={});
json MergeConfigurations(json user_configuration_blob, json target_configuration_blob);


#endif //BUILDTOOL_VCPKGCONFIGURATIONMERGER_H
