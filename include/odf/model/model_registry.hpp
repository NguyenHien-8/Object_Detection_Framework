#pragma once

#include "odf/model/model_spec.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace odf::model {

/** Loads and validates versioned model metadata without loading model binaries. */
class ModelRegistry {
public:
    Status loadDirectory(const std::filesystem::path& root);
    Status add(ModelSpec spec);
    [[nodiscard]] const ModelSpec* find(const std::string& id) const noexcept;
    [[nodiscard]] std::vector<ModelSpec> models() const;
    [[nodiscard]] std::size_t size() const noexcept { return models_.size(); }

private:
    std::map<std::string, ModelSpec> models_;
};

Result<ModelSpec> loadModelSpec(const std::filesystem::path& metadataFile);
Status validateModelSpec(const ModelSpec& spec);

}  // namespace odf::model

