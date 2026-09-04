#pragma once

#include "odf/backend/backend.hpp"
#include "odf/detection/detection.hpp"
#include "odf/image/image.hpp"
#include "odf/model/model_spec.hpp"

#include <optional>

namespace odf::pipeline {

class IDetector {
public:
    virtual ~IDetector() = default;
    virtual Status load(const model::ModelSpec& model,
                        const backend::BackendConfig& backendConfig) = 0;
    virtual Result<detection::DetectionResult> detect(
        const image::ImageFrame& frame,
        const detection::InferenceOptions& options) = 0;
    [[nodiscard]] virtual std::optional<model::ModelSpec> modelInfo() const = 0;
    [[nodiscard]] virtual bool isLoaded() const noexcept = 0;
    virtual void unload() noexcept = 0;
};

}  // namespace odf::pipeline
