#include "odf/models/picodet_adapter.hpp"

#include <cmath>

namespace odf::models {

model::ModelFamily PicoDetAdapter::family() const noexcept {
    return model::ModelFamily::PicoDet;
}

Result<image::PreprocessedInput> PicoDetAdapter::preprocess(
    const image::Image& source,
    const model::ModelSpec& spec) const {
    auto result = image::preprocessImage(source, spec.input, spec.preprocess);
    if (!result.ok()) return result.status();
    for (const auto& name : spec.input.auxiliaryInputs) {
        Tensor tensor;
        tensor.name = name;
        tensor.shape = {1, 2};
        if (name == "im_shape") {
            tensor.data = {static_cast<float>(spec.input.height),
                           static_cast<float>(spec.input.width)};
        } else if (name == "scale_factor") {
            tensor.data = {result.value().transform.scaleY,
                           result.value().transform.scaleX};
        } else {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "unsupported PicoDet auxiliary input='" + name + "'");
        }
        result.value().tensors.push_back(std::move(tensor));
    }
    return result;
}

Result<std::vector<detection::Detection>> PicoDetAdapter::postprocess(
    const std::vector<Tensor>& outputs,
    const image::PreprocessTransform& transform,
    const model::ModelSpec& spec,
    const detection::InferenceOptions& options) const {
    (void)transform;
    if (spec.postprocess.mode != "picodet_paddle_bbox_original" ||
        spec.postprocess.requiresNms) {
        return Status::error(
            ErrorCode::InvalidModelConfig,
            "PicoDet adapter currently requires graph-integrated postprocess mode="
            "picodet_paddle_bbox_original with requires_nms=false");
    }
    if (outputs.empty() || outputs.size() > 2U) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "PicoDet Paddle export must provide bbox and optional bbox_num outputs");
    }
    const auto& boxes = outputs.front();
    if (boxes.shape.size() != 2U || boxes.shape[1] != 6) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "PicoDet bbox output must have shape [N,6]");
    }
    if (outputs.size() == 2U) {
        const auto& count = outputs[1];
        if (count.data.size() != 1U || std::round(count.data[0]) != count.data[0] ||
            count.data[0] < 0.0F || count.data[0] > static_cast<float>(boxes.shape[0])) {
            return Status::error(ErrorCode::TensorShapeMismatch,
                                 "PicoDet bbox_num output is invalid for batch size 1");
        }
    }
    const auto rowCount = outputs.size() == 2U
                              ? static_cast<std::size_t>(outputs[1].data[0])
                              : static_cast<std::size_t>(boxes.shape[0]);
    std::vector<detection::Detection> detections;
    detections.reserve(rowCount);
    for (std::size_t index = 0; index < rowCount; ++index) {
        const float* row = boxes.data.data() + index * 6U;
        for (std::size_t column = 0; column < 6U; ++column) {
            if (!std::isfinite(row[column])) {
                return Status::error(ErrorCode::InferenceFailure,
                                     "PicoDet bbox output contains NaN or infinity");
            }
        }
        const int classId = static_cast<int>(std::round(row[0]));
        if (classId < 0 || classId >= spec.classCount || row[1] < 0.0F ||
            std::fabs(row[0] - static_cast<float>(classId)) > 1.0e-4F || row[1] > 1.0F) {
            return Status::error(ErrorCode::InferenceFailure,
                                 "PicoDet bbox class/confidence value is out of range");
        }
        if (row[1] < options.confidenceThreshold) continue;
        const auto box = detection::BoundingBox{row[2], row[3], row[4], row[5]}.clamped(
            static_cast<float>(transform.originalWidth),
            static_cast<float>(transform.originalHeight));
        if (box.valid()) detections.push_back({box, classId, row[1]});
        if (detections.size() >= options.maxDetections) break;
    }
    return detections;
}

}  // namespace odf::models
