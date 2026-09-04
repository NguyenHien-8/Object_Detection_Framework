#include "odf/app/runtime_catalog.hpp"

#include "odf/models/register_adapters.hpp"

#ifdef ODF_WITH_ONNXRUNTIME
#include "odf/backends/onnxruntime_backend.hpp"
#endif

#include <memory>

namespace odf::app {

RuntimeCatalog::RuntimeCatalog() {
    initializationStatus_ = models::registerTorchAdapters(factory_);
    if (!initializationStatus_.ok()) return;
    initializationStatus_ = models::registerPaddleAdapters(factory_);
    if (!initializationStatus_.ok()) return;
#ifdef ODF_WITH_ONNXRUNTIME
    initializationStatus_ = factory_.registerBackend("onnxruntime", [] {
        return std::make_unique<backends::OnnxRuntimeBackend>();
    });
#endif
}

Status RuntimeCatalog::loadRegistry(const std::filesystem::path& modelRoot) {
    if (!initializationStatus_.ok()) return initializationStatus_;
    return registry_.loadDirectory(modelRoot);
}

std::vector<backend::BackendInfo> RuntimeCatalog::backendInfos() const {
    return factory_.backendInfos();
}

Result<std::unique_ptr<pipeline::IDetector>> RuntimeCatalog::createLoadedDetector(
    const std::string& modelId,
    const std::string& backendName,
    const backend::BackendConfig& config,
    bool allowUnvalidatedModel) const {
    if (!initializationStatus_.ok()) return initializationStatus_;
    const auto* spec = registry_.find(modelId);
    if (spec == nullptr) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model is not present in the registry: " + modelId);
    }
    auto detector = factory_.create(
        *spec, backendName,
        pipeline::DetectorCreationOptions{allowUnvalidatedModel});
    if (!detector.ok()) return detector.status();
    const auto status = detector.value()->load(*spec, config);
    if (!status.ok()) return status;
    return detector.takeValue();
}

}  // namespace odf::app

