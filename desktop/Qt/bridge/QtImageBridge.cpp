#include "bridge/QtImageBridge.hpp"

#include <cstring>

namespace odf::desktop {

Result<QImage> toQImage(const image::Image& source) {
    const auto status = source.validate();
    if (!status.ok()) return status;

    QImage result(source.width, source.height, QImage::Format_RGB888);
    if (result.isNull()) {
        return Status::error(ErrorCode::Internal, "Qt could not allocate the display image");
    }
    const auto sourceStride = static_cast<std::size_t>(source.width) * 3U;
    for (int row = 0; row < source.height; ++row) {
        auto* destination = result.scanLine(row);
        const auto* input = source.pixels.data() + static_cast<std::size_t>(row) * sourceStride;
        if (source.format == image::PixelFormat::Rgb8) {
            std::memcpy(destination, input, sourceStride);
        } else {
            for (int column = 0; column < source.width; ++column) {
                destination[column * 3] = input[column * 3 + 2];
                destination[column * 3 + 1] = input[column * 3 + 1];
                destination[column * 3 + 2] = input[column * 3];
            }
        }
    }
    return result;
}

}  // namespace odf::desktop

