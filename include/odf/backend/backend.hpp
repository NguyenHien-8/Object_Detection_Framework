#pragma once

#include "odf/model/model_spec.hpp"
#include "odf/status.hpp"
#include "odf/tensor.hpp"

#include <string>
#include <vector>

namespace odf::backend {

struct DeviceInfo {
    std::string id;
    std::string displayName;
};

struct BackendInfo {
    std::string name;
    std::string version;
    bool available{false};
    std::string unavailableReason;
    std::vector<DeviceInfo> devices;
    std::vector<model::Precision> precisions;
    std::vector<std::string> modelFormats;
    bool supportsDynamicShapes{false};
    bool supportsCuda{false};
    bool supportsVulkan{false};
};

struct BackendConfig {
    std::string device{"CPU"};
    model::Precision precision{model::Precision::Fp32};
    int intraOpThreads{0};
};

/** Runtime-neutral model execution contract. Instances are serialized by DetectorPipeline. */
class IInferenceBackend {
public:
    virtual ~IInferenceBackend() = default;
    [[nodiscard]] virtual BackendInfo info() const = 0;
    virtual Status load(const model::ModelArtifact& artifact,
                        const BackendConfig& config) = 0;
    virtual Result<std::vector<Tensor>> infer(const std::vector<Tensor>& inputs) = 0;
    virtual void unload() noexcept = 0;
};

}  // namespace odf::backend

