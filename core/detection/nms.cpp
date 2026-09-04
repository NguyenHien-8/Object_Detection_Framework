#include "odf/detection/nms.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace odf::detection {

Result<std::vector<Detection>> nonMaximumSuppression(
    const std::vector<Detection>& detections,
    float iouThreshold,
    std::size_t maxDetections,
    bool classAgnostic) {
    if (!std::isfinite(iouThreshold) || iouThreshold < 0.0F || iouThreshold > 1.0F) {
        return Status::error(ErrorCode::InvalidArgument,
                             "NMS IoU threshold must be finite and in [0, 1]");
    }

    std::vector<std::size_t> order;
    order.reserve(detections.size());
    for (std::size_t i = 0; i < detections.size(); ++i) {
        const auto& detection = detections[i];
        if (detection.box.valid() && detection.classId >= 0 &&
            std::isfinite(detection.confidence) && detection.confidence >= 0.0F &&
            detection.confidence <= 1.0F) {
            order.push_back(i);
        }
    }
    std::stable_sort(order.begin(), order.end(), [&](std::size_t lhs, std::size_t rhs) {
        return detections[lhs].confidence > detections[rhs].confidence;
    });

    std::vector<Detection> kept;
    kept.reserve(std::min(maxDetections, order.size()));
    for (const auto index : order) {
        if (kept.size() >= maxDetections) {
            break;
        }
        const auto& candidate = detections[index];
        bool suppressed = false;
        for (const auto& accepted : kept) {
            if ((classAgnostic || accepted.classId == candidate.classId) &&
                intersectionOverUnion(accepted.box, candidate.box) > iouThreshold) {
                suppressed = true;
                break;
            }
        }
        if (!suppressed) {
            kept.push_back(candidate);
        }
    }
    return kept;
}

std::vector<Detection> filterSelectedClasses(const std::vector<Detection>& detections,
                                             const std::vector<int>& selectedClassIds) {
    if (selectedClassIds.empty()) {
        return detections;
    }
    const std::unordered_set<int> selected(selectedClassIds.begin(), selectedClassIds.end());
    std::vector<Detection> filtered;
    filtered.reserve(detections.size());
    for (const auto& detection : detections) {
        if (selected.count(detection.classId) != 0U) {
            filtered.push_back(detection);
        }
    }
    return filtered;
}

}  // namespace odf::detection

