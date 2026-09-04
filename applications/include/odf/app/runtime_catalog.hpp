#pragma once

#include "odf/backend/backend.hpp"
#include "odf/model/model_registry.hpp"
#include "odf/pipeline/detector_factory.hpp"

#include <filesystem>
#include <memory>

namespace odf::app {

/** Application composition root shared by CLI and Qt clients. */
class RuntimeCatalog {
public:
    RuntimeCatalog();
    Status loadRegistry(const std::filesystem::path& modelRoot);
    [[nodiscard]] const model::ModelRegistry& registry() const noexcept { return registry_; }
    [[nodiscard]] std::vector<backend::BackendInfo> backendInfos() const;
    Result<std::unique_ptr<pipeline::IDetector>> createLoadedDetector(
        const std::string& modelId,
        const std::string& backendName,
        const backend::BackendConfig& config,
        bool allowUnvalidatedModel) const;

private:
    Status initializationStatus_;
    model::ModelRegistry registry_;
    pipeline::DetectorFactory factory_;
};

}  // namespace odf::app

