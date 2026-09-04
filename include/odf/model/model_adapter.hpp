#pragma once

#include "odf/detection/detection.hpp"
#include "odf/image/preprocess.hpp"
#include "odf/model/model_spec.hpp"
#include "odf/status.hpp"
#include "odf/tensor.hpp"

#include <vector>

namespace odf::model {

/** Model-family logic only; runtime ownership belongs to IInferenceBackend. */
class IModelAdapter {
public:
    virtual ~IModelAdapter() = default;
    [[nodiscard]] virtual ModelFamily family() const noexcept = 0;
    virtual Result<image::PreprocessedInput> preprocess(const image::Image& image,
                                                        const ModelSpec& spec) const = 0;
    virtual Result<std::vector<detection::Detection>> postprocess(
        const std::vector<Tensor>& outputs,
        const image::PreprocessTransform& transform,
        const ModelSpec& spec,
        const detection::InferenceOptions& options) const = 0;
};

}  // namespace odf::model

