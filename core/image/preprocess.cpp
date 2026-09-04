#include "odf/image/preprocess.hpp"

#include "odf/detection/bounding_box.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace odf::image {
namespace {

constexpr std::size_t kChannels = 3;

float bilinearSample(const Image& image, float x, float y, int channel) {
    x = std::clamp(x, 0.0F, static_cast<float>(image.width - 1));
    y = std::clamp(y, 0.0F, static_cast<float>(image.height - 1));
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = std::min(x0 + 1, image.width - 1);
    const int y1 = std::min(y0 + 1, image.height - 1);
    const float xWeight = x - static_cast<float>(x0);
    const float yWeight = y - static_cast<float>(y0);
    const auto at = [&](int row, int column) {
        const auto offset = (static_cast<std::size_t>(row) *
                             static_cast<std::size_t>(image.width) +
                             static_cast<std::size_t>(column)) * kChannels +
                            static_cast<std::size_t>(channel);
        return static_cast<float>(image.pixels[offset]);
    };
    const float top = at(y0, x0) * (1.0F - xWeight) + at(y0, x1) * xWeight;
    const float bottom = at(y1, x0) * (1.0F - xWeight) + at(y1, x1) * xWeight;
    return top * (1.0F - yWeight) + bottom * yWeight;
}

}  // namespace

Status Image::validate() const {
    if (width <= 0 || height <= 0) {
        return Status::error(ErrorCode::InvalidArgument,
                             "image width and height must be positive");
    }
    const auto widthValue = static_cast<std::size_t>(width);
    const auto heightValue = static_cast<std::size_t>(height);
    if (heightValue > std::numeric_limits<std::size_t>::max() / widthValue) {
        return Status::error(ErrorCode::InvalidArgument,
                             "image dimensions overflow size_t");
    }
    const auto pixelCount = widthValue * heightValue;
    if (pixelCount > std::numeric_limits<std::size_t>::max() / kChannels ||
        pixels.size() != pixelCount * kChannels) {
        return Status::error(ErrorCode::InvalidArgument,
                             "image payload must contain exactly width*height*3 bytes");
    }
    return Status::success();
}

Result<PreprocessedInput> preprocessImage(const Image& image,
                                         const model::InputSpec& input,
                                         const model::PreprocessConfig& config) {
    const auto imageStatus = image.validate();
    if (!imageStatus.ok()) {
        return imageStatus;
    }
    if (input.width <= 0 || input.height <= 0 || input.width > 16384 ||
        input.height > 16384) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "network dimensions must be in [1, 16384]");
    }
    if (input.layout != model::TensorLayout::Chw ||
        input.dataType != DataType::Float32) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "ODF 0.1 preprocessing supports CHW FP32 inputs only");
    }
    for (const float deviation : config.standardDeviation) {
        if (!std::isfinite(deviation) || deviation == 0.0F) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "preprocess standard deviation must be finite and non-zero");
        }
    }
    if (!std::isfinite(config.scale) || !std::isfinite(config.padValue)) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "preprocess scale and pad value must be finite");
    }

    PreprocessedInput result;
    auto& transform = result.transform;
    transform.originalWidth = image.width;
    transform.originalHeight = image.height;
    transform.networkWidth = input.width;
    transform.networkHeight = input.height;

    int resizedWidth = input.width;
    int resizedHeight = input.height;
    if (config.resize == model::ResizeMode::Letterbox) {
        const float uniformScale = std::min(
            static_cast<float>(input.width) / static_cast<float>(image.width),
            static_cast<float>(input.height) / static_cast<float>(image.height));
        resizedWidth = std::max(1, static_cast<int>(std::round(image.width * uniformScale)));
        resizedHeight = std::max(1, static_cast<int>(std::round(image.height * uniformScale)));
        transform.scaleX = static_cast<float>(resizedWidth) / static_cast<float>(image.width);
        transform.scaleY = static_cast<float>(resizedHeight) / static_cast<float>(image.height);
        transform.padX = static_cast<float>(input.width - resizedWidth) * 0.5F;
        transform.padY = static_cast<float>(input.height - resizedHeight) * 0.5F;
        transform.letterboxed = true;
    } else {
        transform.scaleX = static_cast<float>(input.width) / static_cast<float>(image.width);
        transform.scaleY = static_cast<float>(input.height) / static_cast<float>(image.height);
    }

    Tensor tensor;
    tensor.name = input.name;
    tensor.shape = {1, 3, input.height, input.width};
    tensor.data.resize(kChannels * static_cast<std::size_t>(input.height) *
                       static_cast<std::size_t>(input.width));
    const auto planeSize = static_cast<std::size_t>(input.width) *
                           static_cast<std::size_t>(input.height);
    const int sourceForOutput[3] = {
        input.colorOrder == model::ColorOrder::Rgb && image.format == PixelFormat::Bgr8 ? 2 : 0,
        1,
        input.colorOrder == model::ColorOrder::Rgb && image.format == PixelFormat::Bgr8 ? 0 : 2};
    const bool oppositeOrder = (input.colorOrder == model::ColorOrder::Bgr &&
                                image.format == PixelFormat::Rgb8);

    for (int y = 0; y < input.height; ++y) {
        for (int x = 0; x < input.width; ++x) {
            const float resizedX = static_cast<float>(x) - transform.padX;
            const float resizedY = static_cast<float>(y) - transform.padY;
            const bool padded = resizedX < 0.0F || resizedY < 0.0F ||
                                resizedX >= static_cast<float>(resizedWidth) ||
                                resizedY >= static_cast<float>(resizedHeight);
            const float sourceX = (resizedX + 0.5F) / transform.scaleX - 0.5F;
            const float sourceY = (resizedY + 0.5F) / transform.scaleY - 0.5F;
            for (int channel = 0; channel < 3; ++channel) {
                int sourceChannel = sourceForOutput[channel];
                if (oppositeOrder) {
                    sourceChannel = 2 - channel;
                }
                const float raw = padded ? config.padValue
                                         : bilinearSample(image, sourceX, sourceY, sourceChannel);
                const float normalized =
                    (raw * config.scale - config.mean[static_cast<std::size_t>(channel)]) /
                    config.standardDeviation[static_cast<std::size_t>(channel)];
                const auto destination = static_cast<std::size_t>(channel) * planeSize +
                                         static_cast<std::size_t>(y) *
                                             static_cast<std::size_t>(input.width) +
                                         static_cast<std::size_t>(x);
                tensor.data[destination] = normalized;
            }
        }
    }
    result.tensors.push_back(std::move(tensor));
    return result;
}

detection::BoundingBox restoreBox(const detection::BoundingBox& box,
                                  const PreprocessTransform& transform) noexcept {
    if (!(transform.scaleX > 0.0F) || !(transform.scaleY > 0.0F)) {
        return {};
    }
    detection::BoundingBox restored{
        (box.x1 - transform.padX) / transform.scaleX,
        (box.y1 - transform.padY) / transform.scaleY,
        (box.x2 - transform.padX) / transform.scaleX,
        (box.y2 - transform.padY) / transform.scaleY};
    return restored.clamped(static_cast<float>(transform.originalWidth),
                            static_cast<float>(transform.originalHeight));
}

}  // namespace odf::image
