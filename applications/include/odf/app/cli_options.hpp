#pragma once

#include "odf/status.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace odf::app {

struct CommandLineOptions {
    std::optional<std::filesystem::path> models;
    std::string model{"yolo26n"};
    std::string backend{"onnxruntime"};
    bool modelSpecified{false};
    bool backendSpecified{false};
    std::optional<std::filesystem::path> image;
    std::optional<std::filesystem::path> output;
    std::optional<int> camera;
    std::string logLevel{"info"};
    float confidence{0.25F};
    float iou{0.45F};
    std::size_t maxDetections{300};
    bool confidenceSpecified{false};
    bool iouSpecified{false};
    bool maxDetectionsSpecified{false};
    bool allowUnvalidatedModel{false};
    bool smokeTest{false};
    bool help{false};
};

Result<CommandLineOptions> parseCommandLine(const std::vector<std::string>& arguments);
std::string commandLineUsage(const std::string& executableName);

}  // namespace odf::app
