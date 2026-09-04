#pragma once

#include "odf/detection/detection.hpp"
#include "odf/image/image.hpp"
#include "odf/status.hpp"

#include <opencv2/core/mat.hpp>

#include <string>
#include <vector>

namespace odf::app {

Result<image::Image> toOdfImage(const cv::Mat& source);
Result<cv::Mat> toCvImage(const image::Image& source);
Result<cv::Mat> renderDetections(const cv::Mat& original,
                                 const std::vector<detection::Detection>& detections,
                                 const std::vector<std::string>& labels);

}  // namespace odf::app

