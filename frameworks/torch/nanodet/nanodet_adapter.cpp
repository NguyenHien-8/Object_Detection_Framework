#include "odf/models/nanodet_adapter.hpp"

#include "odf/detection/nms.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace odf::models {
namespace {

Result<float> distributionExpectation(const float* logits, int bins) {
    float maximum = -std::numeric_limits<float>::infinity();
    for (int index = 0; index < bins; ++index) {
        if (!std::isfinite(logits[index])) {
            return Status::error(ErrorCode::InferenceFailure,
                                 "NanoDet regression output contains NaN or infinity");
        }
        maximum = std::max(maximum, logits[index]);
    }
    float denominator = 0.0F;
    float numerator = 0.0F;
    for (int index = 0; index < bins; ++index) {
        const float probability = std::exp(logits[index] - maximum);
        denominator += probability;
        numerator += probability * static_cast<float>(index);
    }
    if (!(denominator > 0.0F) || !std::isfinite(denominator)) {
        return Status::error(ErrorCode::InferenceFailure,
                             "NanoDet regression distribution is invalid");
    }
    return numerator / denominator;
}

}  // namespace

model::ModelFamily NanoDetAdapter::family() const noexcept {
    return model::ModelFamily::NanoDet;
}

Result<image::PreprocessedInput> NanoDetAdapter::preprocess(
    const image::Image& source,
    const model::ModelSpec& spec) const {
    return image::preprocessImage(source, spec.input, spec.preprocess);
}

Result<std::vector<detection::Detection>> NanoDetAdapter::postprocess(
    const std::vector<Tensor>& outputs,
    const image::PreprocessTransform& transform,
    const model::ModelSpec& spec,
    const detection::InferenceOptions& options) const {
    if (spec.postprocess.mode != "nanodet_plus_dfl" ||
        !spec.postprocess.requiresNms) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "NanoDet requires mode=nanodet_plus_dfl and requires_nms=true");
    }
    if (outputs.size() != 1U) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "NanoDet concatenated export requires exactly one output tensor");
    }
    const Tensor& output = outputs.front();
    if (output.dataType != DataType::Float32 ||
        (output.shape.size() != 2U && output.shape.size() != 3U)) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "NanoDet output shape must be [points,channels] or [1,points,channels]");
    }
    const std::size_t offset = output.shape.size() == 3U ? 1U : 0U;
    if (output.shape.size() == 3U && output.shape[0] != 1) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "NanoDet adapter currently supports batch size 1 only");
    }
    if (spec.postprocess.regressionMax < 1 || spec.postprocess.regressionMax > 32 ||
        spec.postprocess.strides.empty()) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "NanoDet reg_max must be in [1,32] and strides cannot be empty");
    }
    const auto bins = spec.postprocess.regressionMax + 1;
    const auto expectedChannels = spec.classCount + 4 * bins;
    const auto pointCount = static_cast<std::size_t>(output.shape[offset]);
    if (output.shape[offset + 1U] != expectedChannels) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "NanoDet output channel count does not match class_count + 4*(reg_max+1)");
    }
    std::size_t expectedPoints = 0;
    for (const int stride : spec.postprocess.strides) {
        if (stride <= 0 || spec.input.width % stride != 0 || spec.input.height % stride != 0) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "NanoDet strides must be positive divisors of the input size");
        }
        expectedPoints += static_cast<std::size_t>(spec.input.width / stride) *
                          static_cast<std::size_t>(spec.input.height / stride);
    }
    if (pointCount != expectedPoints) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "NanoDet output point count is inconsistent with input size and strides");
    }

    std::vector<detection::Detection> decoded;
    std::size_t pointIndex = 0;
    for (const int stride : spec.postprocess.strides) {
        const int columns = spec.input.width / stride;
        const int rows = spec.input.height / stride;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < columns; ++x, ++pointIndex) {
                const float* row = output.data.data() + pointIndex *
                                   static_cast<std::size_t>(expectedChannels);
                int classId = 0;
                float confidence = row[0];
                for (int classIndex = 0; classIndex < spec.classCount; ++classIndex) {
                    if (!std::isfinite(row[classIndex])) {
                        return Status::error(ErrorCode::InferenceFailure,
                                             "NanoDet class output contains NaN or infinity");
                    }
                    if (row[classIndex] < 0.0F || row[classIndex] > 1.0F) {
                        return Status::error(ErrorCode::InferenceFailure,
                                             "NanoDet class probability is outside [0,1]");
                    }
                    if (row[classIndex] > confidence) {
                        confidence = row[classIndex];
                        classId = classIndex;
                    }
                }
                if (confidence < options.confidenceThreshold) {
                    continue;
                }
                float distances[4]{};
                for (int side = 0; side < 4; ++side) {
                    auto expectation = distributionExpectation(
                        row + spec.classCount + side * bins, bins);
                    if (!expectation.ok()) return expectation.status();
                    distances[side] = expectation.value() * static_cast<float>(stride);
                }
                const float centerX = (static_cast<float>(x) + 0.5F) * stride;
                const float centerY = (static_cast<float>(y) + 0.5F) * stride;
                const detection::BoundingBox networkBox{centerX - distances[0],
                                                        centerY - distances[1],
                                                        centerX + distances[2],
                                                        centerY + distances[3]};
                const auto restored = image::restoreBox(networkBox, transform);
                if (restored.valid()) {
                    decoded.push_back({restored, classId, confidence});
                }
            }
        }
    }
    return detection::nonMaximumSuppression(decoded, options.iouThreshold,
                                             options.maxDetections,
                                             options.classAgnosticNms);
}

}  // namespace odf::models
