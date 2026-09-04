#include "odf/pipeline/detector_pipeline.hpp"

#include "odf/detection/nms.hpp"
#include "odf/logging.hpp"
#include "odf/model/model_registry.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>

namespace odf::pipeline {
namespace {
double milliseconds(std::chrono::steady_clock::duration duration) {
    return std::chrono::duration<double, std::milli>(duration).count();
}

Status validateOptions(const detection::InferenceOptions& options) {
    if (!std::isfinite(options.confidenceThreshold) || options.confidenceThreshold < 0.0F ||
        options.confidenceThreshold > 1.0F || !std::isfinite(options.iouThreshold) ||
        options.iouThreshold < 0.0F || options.iouThreshold > 1.0F) {
        return Status::error(ErrorCode::InvalidArgument,
                             "confidence and IoU thresholds must be finite and in [0, 1]");
    }
    return Status::success();
}
}  // namespace

DetectorPipeline::DetectorPipeline(std::unique_ptr<model::IModelAdapter> adapter,
                                   std::unique_ptr<backend::IInferenceBackend> backend)
    : adapter_(std::move(adapter)), backend_(std::move(backend)) {
    if (!adapter_ || !backend_) {
        throw std::invalid_argument("DetectorPipeline requires an adapter and backend");
    }
}

DetectorPipeline::~DetectorPipeline() { unload(); }

Status DetectorPipeline::load(const model::ModelSpec& spec,
                              const backend::BackendConfig& backendConfig) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto modelStatus = model::validateModelSpec(spec);
    if (!modelStatus.ok()) {
        return modelStatus;
    }
    if (!spec.deploymentValidated) {
        return Status::error(
            ErrorCode::InvalidModelConfig,
            "model='" + spec.id +
                "' is a registry template; verify it against an exported artifact before loading");
    }
    if (adapter_->family() != spec.family) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "adapter family does not match model='" + spec.id + "'");
    }
    const auto info = backend_->info();
    if (!info.available) {
        return Status::error(ErrorCode::BackendUnavailable,
                             "backend='" + info.name + "' unavailable: " +
                                 info.unavailableReason);
    }
    const auto artifact = spec.artifacts.find(info.name);
    if (artifact == spec.artifacts.end()) {
        return Status::error(ErrorCode::UnsupportedModelBackendPair,
                             "model='" + spec.id + "' has no artifact for backend='" +
                                 info.name + "'");
    }

    backend_->unload();
    model_.reset();
    const auto loadStatus = backend_->load(artifact->second, backendConfig);
    if (!loadStatus.ok()) {
        return loadStatus;
    }
    model_ = std::make_unique<model::ModelSpec>(spec);
    ++generation_;
    logging::log(logging::Level::Info,
                 "loaded model='" + spec.id + "' backend='" + info.name + "'");
    return Status::success();
}

Result<detection::DetectionResult> DetectorPipeline::detect(
    const image::ImageFrame& frame,
    const detection::InferenceOptions& options) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!model_) {
        return Status::error(ErrorCode::InferenceFailure,
                             "detect called before a model was loaded");
    }
    const auto optionStatus = validateOptions(options);
    if (!optionStatus.ok()) {
        return optionStatus;
    }
    std::vector<int> selectedClassIds = options.selectedClassIds;
    std::sort(selectedClassIds.begin(), selectedClassIds.end());
    if (std::adjacent_find(selectedClassIds.begin(), selectedClassIds.end()) !=
            selectedClassIds.end() ||
        std::any_of(selectedClassIds.begin(), selectedClassIds.end(), [&](int classId) {
            return classId < 0 || classId >= model_->classCount;
        })) {
        return Status::error(ErrorCode::InvalidArgument,
                             "selected class IDs must be unique and in the model class range");
    }
    const auto imageStatus = frame.image.validate();
    if (!imageStatus.ok()) {
        return imageStatus;
    }

    try {
        const auto totalStart = std::chrono::steady_clock::now();
        const auto preprocessStart = totalStart;
        auto preprocessed = adapter_->preprocess(frame.image, *model_);
        const auto preprocessEnd = std::chrono::steady_clock::now();
        if (!preprocessed.ok()) {
            return preprocessed.status();
        }

        const auto inferenceStart = preprocessEnd;
        auto outputs = backend_->infer(preprocessed.value().tensors);
        const auto inferenceEnd = std::chrono::steady_clock::now();
        if (!outputs.ok()) {
            return outputs.status();
        }
        for (const auto& output : outputs.value()) {
            const auto status = output.validate();
            if (!status.ok()) {
                return status;
            }
        }

        const auto postprocessStart = inferenceEnd;
        auto detections = adapter_->postprocess(outputs.value(),
                                                preprocessed.value().transform,
                                                *model_, options);
        if (!detections.ok()) {
            return detections.status();
        }
        auto selected = detection::filterSelectedClasses(detections.value(),
                                                         options.selectedClassIds);
        if (selected.size() > options.maxDetections) {
            selected.resize(options.maxDetections);
        }
        const auto postprocessEnd = std::chrono::steady_clock::now();

        detection::DetectionResult result;
        result.detections = std::move(selected);
        result.frameId = frame.frameId;
        result.modelGeneration = generation_;
        result.modelId = model_->id;
        result.timings.preprocessMs = milliseconds(preprocessEnd - preprocessStart);
        result.timings.inferenceMs = milliseconds(inferenceEnd - inferenceStart);
        result.timings.postprocessMs = milliseconds(postprocessEnd - postprocessStart);
        result.timings.totalMs = milliseconds(postprocessEnd - totalStart);
        return result;
    } catch (const std::exception& exception) {
        return Status::error(ErrorCode::InferenceFailure,
                             "model='" + model_->id + "' inference exception: " +
                                 exception.what());
    } catch (...) {
        return Status::error(ErrorCode::InferenceFailure,
                             "model='" + model_->id + "' inference raised an unknown exception");
    }
}

std::optional<model::ModelSpec> DetectorPipeline::modelInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return model_ ? std::optional<model::ModelSpec>(*model_) : std::nullopt;
}

bool DetectorPipeline::isLoaded() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return model_ != nullptr;
}

void DetectorPipeline::unload() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    backend_->unload();
    model_.reset();
}

bool isCurrentResult(const detection::DetectionResult& result,
                     std::uint64_t expectedFrame,
                     std::uint64_t expectedGeneration,
                     const std::string& expectedModelId) noexcept {
    return result.frameId == expectedFrame && result.modelGeneration == expectedGeneration &&
           result.modelId == expectedModelId;
}

}  // namespace odf::pipeline
