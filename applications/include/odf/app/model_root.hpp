#pragma once

#include "odf/status.hpp"

#include <filesystem>
#include <optional>

namespace odf::app {

struct ModelRootSearch {
    std::optional<std::filesystem::path> commandLine;
    std::optional<std::filesystem::path> environment;
    std::filesystem::path applicationDirectory;
    std::optional<std::filesystem::path> installedShare;
};

/** Resolves CLI, environment, application-local, then installed model roots. */
Result<std::filesystem::path> discoverModelRoot(const ModelRootSearch& search);
std::optional<std::filesystem::path> modelRootFromEnvironment();
std::filesystem::path compiledInstalledModelRoot();

}  // namespace odf::app

