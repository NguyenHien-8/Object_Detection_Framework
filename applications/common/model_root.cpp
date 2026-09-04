#include "odf/app/model_root.hpp"

#include <cstdlib>

namespace odf::app {
namespace {

bool containsMetadata(const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::is_directory(path, error)) return false;
    std::filesystem::recursive_directory_iterator iterator(
        path, std::filesystem::directory_options::skip_permission_denied, error);
    const std::filesystem::recursive_directory_iterator end;
    while (!error && iterator != end) {
        if (iterator->is_regular_file(error) && iterator->path().filename() == "model.json") {
            return true;
        }
        iterator.increment(error);
    }
    return false;
}

Result<std::filesystem::path> validateExplicit(const std::filesystem::path& path,
                                               const char* source) {
    if (!containsMetadata(path)) {
        return Status::error(ErrorCode::FileNotFound,
                             std::string(source) + " model root contains no model.json: " +
                                 path.string());
    }
    std::error_code error;
    const auto canonical = std::filesystem::weakly_canonical(path, error);
    return error ? std::filesystem::absolute(path) : canonical;
}

}  // namespace

Result<std::filesystem::path> discoverModelRoot(const ModelRootSearch& search) {
    if (search.commandLine) return validateExplicit(*search.commandLine, "command-line");
    if (search.environment) return validateExplicit(*search.environment, "ODF_MODEL_ROOT");
    const auto local = search.applicationDirectory / "models";
    if (containsMetadata(local)) return validateExplicit(local, "application-local");
    if (search.installedShare && containsMetadata(*search.installedShare)) {
        return validateExplicit(*search.installedShare, "installed");
    }
    return Status::error(
        ErrorCode::FileNotFound,
        "model registry not found; use --models, ODF_MODEL_ROOT, <application>/models, or install it");
}

std::optional<std::filesystem::path> modelRootFromEnvironment() {
#ifdef _WIN32
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, "ODF_MODEL_ROOT") != 0 || value == nullptr) {
        return std::nullopt;
    }
    std::optional<std::filesystem::path> result;
    if (*value != '\0') result = std::filesystem::path(value);
    std::free(value);
    return result;
#else
    const char* value = std::getenv("ODF_MODEL_ROOT");
    if (value == nullptr || *value == '\0') return std::nullopt;
    return std::filesystem::path(value);
#endif
}

std::filesystem::path compiledInstalledModelRoot() { return ODF_INSTALLED_MODEL_DIR; }

}  // namespace odf::app
