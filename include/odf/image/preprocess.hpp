#pragma once

#include "odf/detection/bounding_box.hpp"
#include "odf/image/image.hpp"
#include "odf/model/model_spec.hpp"
#include "odf/tensor.hpp"

namespace odf::image {

struct PreprocessedInput {
    std::vector<Tensor> tensors;
    PreprocessTransform transform;
};

/** Resizes, converts color, normalizes and emits a contiguous CHW FP32 tensor. */
Result<PreprocessedInput> preprocessImage(const Image& image,
                                         const model::InputSpec& input,
                                         const model::PreprocessConfig& config);

/** Maps a network-space box back to original-image coordinates. */
detection::BoundingBox restoreBox(const detection::BoundingBox& box,
                                  const PreprocessTransform& transform) noexcept;

}  // namespace odf::image
