#pragma once

#include "odf/backend/backend.hpp"

#include <memory>

namespace odf::backends {

/** ONNX Runtime CPU backend. ONNX Runtime types are hidden behind PImpl. */
class OnnxRuntimeBackend final : public backend::IInferenceBackend {
public:
    OnnxRuntimeBackend();
    ~OnnxRuntimeBackend() override;
    OnnxRuntimeBackend(OnnxRuntimeBackend&&) noexcept;
    OnnxRuntimeBackend& operator=(OnnxRuntimeBackend&&) noexcept;
    OnnxRuntimeBackend(const OnnxRuntimeBackend&) = delete;
    OnnxRuntimeBackend& operator=(const OnnxRuntimeBackend&) = delete;

    [[nodiscard]] backend::BackendInfo info() const override;
    Status load(const model::ModelArtifact& artifact,
                const backend::BackendConfig& config) override;
    Result<std::vector<Tensor>> infer(const std::vector<Tensor>& inputs) override;
    void unload() noexcept override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace odf::backends

