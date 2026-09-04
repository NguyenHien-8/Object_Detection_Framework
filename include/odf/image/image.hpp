#pragma once

#include "odf/status.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace odf::image {

enum class PixelFormat { Bgr8, Rgb8 };

/** Owned interleaved 8-bit image, independent from GUI and runtime libraries. */
struct Image {
    int width{0};
    int height{0};
    PixelFormat format{PixelFormat::Bgr8};
    std::vector<std::uint8_t> pixels;

    [[nodiscard]] Status validate() const;
};

struct ImageFrame {
    Image image;
    std::chrono::steady_clock::time_point timestamp;
    std::uint64_t frameId{0};
    std::string source;
};

struct PreprocessTransform {
    int originalWidth{0};
    int originalHeight{0};
    int networkWidth{0};
    int networkHeight{0};
    float scaleX{1.0F};
    float scaleY{1.0F};
    float padX{0.0F};
    float padY{0.0F};
    bool letterboxed{false};
};

}  // namespace odf::image

