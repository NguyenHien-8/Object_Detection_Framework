#include "odf/pipeline/detector_factory.hpp"

#include "odf/pipeline/detector_pipeline.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace odf::pipeline {

Status DetectorFactory::registerAdapter(model::ModelFamily family, AdapterCreator creator) {
    if (!creator) {
        return Status::error(ErrorCode::InvalidArgument, "adapter creator is empty");
    }
    if (adapters_.count(family) != 0U) {
        return Status::error(ErrorCode::InvalidArgument,
                             std::string("adapter already registered for ") +
                                 model::modelFamilyName(family));
    }
    adapters_.emplace(family, std::move(creator));
    return Status::success();
}

Status DetectorFactory::registerBackend(std::string name, BackendCreator creator) {
    if (name.empty() || !creator) {
        return Status::error(ErrorCode::InvalidArgument,
                             "backend name and creator are required");
    }
    if (backends_.count(name) != 0U) {
        return Status::error(ErrorCode::InvalidArgument,
                             "backend already registered: " + name);
    }
    backends_.emplace(std::move(name), std::move(creator));
    return Status::success();
}

Result<std::unique_ptr<IDetector>> DetectorFactory::create(
    const model::ModelSpec& spec,
    const std::string& backendName,
    DetectorCreationOptions options) const {
    const auto supported = std::find(spec.supportedBackends.begin(),
                                     spec.supportedBackends.end(), backendName);
    if (supported == spec.supportedBackends.end()) {
        return Status::error(ErrorCode::UnsupportedModelBackendPair,
                             "model='" + spec.id + "' does not declare backend='" +
                                 backendName + "'");
    }
    const auto adapter = adapters_.find(spec.family);
    if (adapter == adapters_.end()) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             std::string("no adapter registered for family=") +
                                 model::modelFamilyName(spec.family));
    }
    const auto backend = backends_.find(backendName);
    if (backend == backends_.end()) {
        return Status::error(ErrorCode::UnsupportedBackend,
                             "backend is not registered in this build: " + backendName);
    }
    std::unique_ptr<IDetector> detector = std::make_unique<DetectorPipeline>(
        adapter->second(), backend->second(), options.allowUnvalidatedModel);
    return std::move(detector);
}

std::vector<backend::BackendInfo> DetectorFactory::backendInfos() const {
    std::vector<backend::BackendInfo> result;
    result.reserve(backends_.size());
    for (const auto& entry : backends_) {
        try {
            auto instance = entry.second();
            if (instance) {
                result.push_back(instance->info());
            }
        } catch (const std::exception& exception) {
            backend::BackendInfo info;
            info.name = entry.first;
            info.available = false;
            info.unavailableReason = std::string("capability query failed: ") + exception.what();
            result.push_back(std::move(info));
        } catch (...) {
            backend::BackendInfo info;
            info.name = entry.first;
            info.available = false;
            info.unavailableReason = "capability query failed with an unknown exception";
            result.push_back(std::move(info));
        }
    }
    return result;
}

}  // namespace odf::pipeline
