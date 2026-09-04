#pragma once

#include "odf/model/model_adapter.hpp"

namespace odf::models {

/** Parser for PicoDet Paddle exports with graph-integrated post-processing. */
class PicoDetAdapter final : public model::IModelAdapter {
public:
    [[nodiscard]] model::ModelFamily family() const noexcept override;
    Result<image::PreprocessedInput> preprocess(const image::Image& image,
                                                const model::ModelSpec& spec) const override;
    Result<std::vector<detection::Detection>> postprocess(
        const std::vector<Tensor>& outputs,
        const image::PreprocessTransform& transform,
        const model::ModelSpec& spec,
        const detection::InferenceOptions& options) const override;
};

}  // namespace odf::models

