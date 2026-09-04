#pragma once

#include "odf/detection/detection.hpp"
#include "odf/status.hpp"

#include <vector>

namespace odf::detection {

/** Deterministic greedy NMS sorted by confidence and stable input order. */
Result<std::vector<Detection>> nonMaximumSuppression(
    const std::vector<Detection>& detections,
    float iouThreshold,
    std::size_t maxDetections,
    bool classAgnostic);

/** Applies the selected-class filter without altering neural-network execution. */
std::vector<Detection> filterSelectedClasses(const std::vector<Detection>& detections,
                                             const std::vector<int>& selectedClassIds);

}  // namespace odf::detection

