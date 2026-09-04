#include "odf/app/opencv_bridge.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace odf::app {
namespace {

cv::Scalar classColor(int classId) {
    const auto value = static_cast<unsigned int>(classId) * 2654435761U;
    return {static_cast<double>(64 + static_cast<int>(value & 127U)),
            static_cast<double>(64 + static_cast<int>((value >> 8U) & 127U)),
            static_cast<double>(64 + static_cast<int>((value >> 16U) & 127U))};
}

}  // namespace

Result<image::Image> toOdfImage(const cv::Mat& source) {
    if (source.empty()) {
        return Status::error(ErrorCode::InvalidArgument, "OpenCV image is empty");
    }
    if (source.type() != CV_8UC3) {
        return Status::error(ErrorCode::InvalidArgument,
                             "OpenCV bridge requires an 8-bit three-channel BGR image");
    }
    image::Image result;
    result.width = source.cols;
    result.height = source.rows;
    result.format = image::PixelFormat::Bgr8;
    const auto rowBytes = static_cast<std::size_t>(source.cols) * 3U;
    result.pixels.resize(rowBytes * static_cast<std::size_t>(source.rows));
    for (int row = 0; row < source.rows; ++row) {
        std::memcpy(result.pixels.data() + static_cast<std::size_t>(row) * rowBytes,
                    source.ptr(row), rowBytes);
    }
    return result;
}

Result<cv::Mat> toCvImage(const image::Image& source) {
    const auto status = source.validate();
    if (!status.ok()) return status;
    cv::Mat wrapped(source.height, source.width, CV_8UC3,
                    const_cast<std::uint8_t*>(source.pixels.data()));
    cv::Mat result = wrapped.clone();
    if (source.format == image::PixelFormat::Rgb8) {
        cv::cvtColor(result, result, cv::COLOR_RGB2BGR);
    }
    return result;
}

Result<cv::Mat> renderDetections(const cv::Mat& original,
                                 const std::vector<detection::Detection>& detections,
                                 const std::vector<std::string>& labels) {
    if (original.empty() || original.type() != CV_8UC3) {
        return Status::error(ErrorCode::InvalidArgument,
                             "rendering requires a non-empty CV_8UC3 image");
    }
    cv::Mat rendered = original.clone();
    for (const auto& detection : detections) {
        if (!detection.box.valid() || detection.classId < 0 ||
            static_cast<std::size_t>(detection.classId) >= labels.size()) {
            return Status::error(ErrorCode::InvalidArgument,
                                 "detection is invalid for the supplied labels");
        }
        const auto color = classColor(detection.classId);
        const cv::Point topLeft(static_cast<int>(std::round(detection.box.x1)),
                                static_cast<int>(std::round(detection.box.y1)));
        const cv::Point bottomRight(static_cast<int>(std::round(detection.box.x2)),
                                   static_cast<int>(std::round(detection.box.y2)));
        cv::rectangle(rendered, topLeft, bottomRight, color, 2, cv::LINE_AA);
        const std::string text = labels[static_cast<std::size_t>(detection.classId)] + " " +
                                 cv::format("%.2f", detection.confidence);
        int baseline = 0;
        const auto textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        const int textY = std::max(textSize.height + 4, topLeft.y);
        cv::rectangle(rendered,
                      {topLeft.x, textY - textSize.height - 4},
                      {topLeft.x + textSize.width + 4, textY + baseline}, color, cv::FILLED);
        cv::putText(rendered, text, {topLeft.x + 2, textY - 2},
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, {255, 255, 255}, 1, cv::LINE_AA);
    }
    return rendered;
}

}  // namespace odf::app
