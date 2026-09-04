#include "odf/models/yolo26_adapter.hpp"

#include "odf/detection/nms.hpp"

#include <algorithm>
#include <cmath>

namespace odf::models {
namespace {

Result<std::vector<detection::Detection>> parseEndToEnd(
    const Tensor& output,
    const image::PreprocessTransform& transform,
    const model::ModelSpec& spec,
    const detection::InferenceOptions& options) {
    if (spec.postprocess.requiresNms) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "YOLO26 end-to-end metadata must set requires_nms=false");
    }
    const bool batched = output.shape.size() == 3U;
    if ((!batched && output.shape.size() != 2U) ||
        (batched && output.shape[0] != 1) || output.shape.back() != 6) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "YOLO26 end-to-end output must have shape [N,6] or [1,N,6]");
    }
    const auto rows = static_cast<std::size_t>(output.shape[batched ? 1U : 0U]);
    std::vector<detection::Detection> detections;
    detections.reserve(std::min(rows, options.maxDetections));
    for (std::size_t rowIndex = 0; rowIndex < rows; ++rowIndex) {
        const float* row = output.data.data() + rowIndex * 6U;
        for (std::size_t column = 0; column < 6U; ++column) {
            if (!std::isfinite(row[column])) {
                return Status::error(ErrorCode::InferenceFailure,
                                     "YOLO26 end-to-end output contains NaN or infinity");
            }
        }
        if (row[4] < 0.0F || row[4] > 1.0F) {
            return Status::error(ErrorCode::InferenceFailure,
                                 "YOLO26 end-to-end confidence is outside [0,1]");
        }
        if (row[4] < options.confidenceThreshold) continue;
        const int classId = static_cast<int>(std::round(row[5]));
        if (std::fabs(row[5] - static_cast<float>(classId)) > 1.0e-4F || classId < 0 ||
            classId >= spec.classCount || row[4] > 1.0F) {
            return Status::error(ErrorCode::InferenceFailure,
                                 "YOLO26 end-to-end class/confidence value is out of range");
        }
        const auto box = image::restoreBox({row[0], row[1], row[2], row[3]}, transform);
        if (box.valid()) detections.push_back({box, classId, row[4]});
        if (detections.size() >= options.maxDetections) break;
    }
    return detections;
}

Result<std::vector<detection::Detection>> parseOneToMany(
    const Tensor& output,
    const image::PreprocessTransform& transform,
    const model::ModelSpec& spec,
    const detection::InferenceOptions& options) {
    if (!spec.postprocess.requiresNms) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "YOLO26 one-to-many metadata must set requires_nms=true");
    }
    const bool batched = output.shape.size() == 3U;
    if ((!batched && output.shape.size() != 2U) || (batched && output.shape[0] != 1)) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "YOLO26 one-to-many output must be [4+C,N] or [1,4+C,N]");
    }
    const std::size_t channelIndex = batched ? 1U : 0U;
    const auto channels = output.shape[channelIndex];
    const auto predictionCount = output.shape[channelIndex + 1U];
    if (channels != spec.classCount + 4 || predictionCount <= 0) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "YOLO26 one-to-many shape must be [1,4+class_count,predictions]");
    }
    const auto predictions = static_cast<std::size_t>(predictionCount);
    std::vector<detection::Detection> candidates;
    candidates.reserve(predictions);
    const auto value = [&](int channel, std::size_t prediction) {
        return output.data[static_cast<std::size_t>(channel) * predictions + prediction];
    };
    for (std::size_t prediction = 0; prediction < predictions; ++prediction) {
        int classId = 0;
        float confidence = value(4, prediction);
        for (int classIndex = 0; classIndex < spec.classCount; ++classIndex) {
            const float score = value(4 + classIndex, prediction);
            if (!std::isfinite(score)) {
                return Status::error(ErrorCode::InferenceFailure,
                                     "YOLO26 one-to-many score contains NaN or infinity");
            }
            if (score < 0.0F || score > 1.0F) {
                return Status::error(ErrorCode::InferenceFailure,
                                     "YOLO26 one-to-many score is outside [0,1]");
            }
            if (score > confidence) {
                confidence = score;
                classId = classIndex;
            }
        }
        if (confidence < options.confidenceThreshold) continue;
        const float centerX = value(0, prediction);
        const float centerY = value(1, prediction);
        const float width = value(2, prediction);
        const float height = value(3, prediction);
        if (!std::isfinite(centerX) || !std::isfinite(centerY) || !std::isfinite(width) ||
            !std::isfinite(height) || width <= 0.0F || height <= 0.0F) {
            continue;
        }
        const auto box = image::restoreBox(
            {centerX - width * 0.5F, centerY - height * 0.5F,
             centerX + width * 0.5F, centerY + height * 0.5F}, transform);
        if (box.valid()) candidates.push_back({box, classId, confidence});
    }
    return detection::nonMaximumSuppression(candidates, options.iouThreshold,
                                             options.maxDetections,
                                             options.classAgnosticNms);
}

}  // namespace

model::ModelFamily Yolo26Adapter::family() const noexcept {
    return model::ModelFamily::Yolo26;
}

Result<image::PreprocessedInput> Yolo26Adapter::preprocess(
    const image::Image& source,
    const model::ModelSpec& spec) const {
    return image::preprocessImage(source, spec.input, spec.preprocess);
}

Result<std::vector<detection::Detection>> Yolo26Adapter::postprocess(
    const std::vector<Tensor>& outputs,
    const image::PreprocessTransform& transform,
    const model::ModelSpec& spec,
    const detection::InferenceOptions& options) const {
    if (outputs.size() != 1U) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "YOLO26 detection exports require exactly one output tensor");
    }
    if (spec.postprocess.mode == "yolo26_end2end") {
        return parseEndToEnd(outputs.front(), transform, spec, options);
    }
    if (spec.postprocess.mode == "yolo26_one_to_many") {
        return parseOneToMany(outputs.front(), transform, spec, options);
    }
    return Status::error(ErrorCode::InvalidModelConfig,
                         "unsupported YOLO26 postprocess mode='" +
                             spec.postprocess.mode + "'");
}

}  // namespace odf::models
