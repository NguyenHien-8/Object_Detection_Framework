#pragma once

#include "odf/backend/backend.hpp"
#include "odf/model/model_adapter.hpp"
#include "odf/pipeline/detector.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace odf::pipeline {

struct DetectorCreationOptions {
    bool allowUnvalidatedModel{false};
};

/** Extensible composition registry below the GUI/application layer. */
class DetectorFactory {
public:
    using AdapterCreator = std::function<std::unique_ptr<model::IModelAdapter>()>;
    using BackendCreator = std::function<std::unique_ptr<backend::IInferenceBackend>()>;

    Status registerAdapter(model::ModelFamily family, AdapterCreator creator);
    Status registerBackend(std::string name, BackendCreator creator);
    Result<std::unique_ptr<IDetector>> create(const model::ModelSpec& spec,
                                              const std::string& backendName,
                                              DetectorCreationOptions options = {}) const;
    [[nodiscard]] std::vector<backend::BackendInfo> backendInfos() const;

private:
    std::map<model::ModelFamily, AdapterCreator> adapters_;
    std::map<std::string, BackendCreator> backends_;
};

}  // namespace odf::pipeline
