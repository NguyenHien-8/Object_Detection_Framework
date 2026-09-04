#pragma once

#include "odf/model/model_adapter.hpp"
#include "odf/pipeline/detector.hpp"

#include <memory>
#include <mutex>

namespace odf::pipeline {

/** Serialized detector that composes one adapter with one runtime backend. */
class DetectorPipeline final : public IDetector {
public:
    DetectorPipeline(std::unique_ptr<model::IModelAdapter> adapter,
                     std::unique_ptr<backend::IInferenceBackend> backend,
                     bool allowUnvalidatedModel = false);
    ~DetectorPipeline() override;

    Status load(const model::ModelSpec& model,
                const backend::BackendConfig& backendConfig) override;
    Result<detection::DetectionResult> detect(
        const image::ImageFrame& frame,
        const detection::InferenceOptions& options) override;
    [[nodiscard]] std::optional<model::ModelSpec> modelInfo() const override;
    [[nodiscard]] bool isLoaded() const noexcept override;
    void unload() noexcept override;

private:
    std::unique_ptr<model::IModelAdapter> adapter_;
    std::unique_ptr<backend::IInferenceBackend> backend_;
    mutable std::mutex mutex_;
    std::unique_ptr<model::ModelSpec> model_;
    std::uint64_t generation_{0};
    bool allowUnvalidatedModel_{false};
};

/** Returns false for results produced before the current model generation. */
bool isCurrentResult(const detection::DetectionResult& result,
                     std::uint64_t expectedFrame,
                     std::uint64_t expectedGeneration,
                     const std::string& expectedModelId) noexcept;

}  // namespace odf::pipeline
