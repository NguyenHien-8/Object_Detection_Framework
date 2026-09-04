#pragma once

#include "odf/detection/bounding_box.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace odf::detection {

struct Detection {
    BoundingBox box;
    int classId{-1};
    float confidence{0.0F};
};

struct InferenceOptions {
    float confidenceThreshold{0.25F};
    float iouThreshold{0.45F};
    std::size_t maxDetections{300};
    std::vector<int> selectedClassIds;
    bool classAgnosticNms{false};
};

struct StageTimings {
    double preprocessMs{0.0};
    double inferenceMs{0.0};
    double postprocessMs{0.0};
    double totalMs{0.0};
};

struct DetectionResult {
    std::vector<Detection> detections;
    StageTimings timings;
    std::uint64_t frameId{0};
    std::uint64_t modelGeneration{0};
    std::string modelId;
};

}  // namespace odf::detection
