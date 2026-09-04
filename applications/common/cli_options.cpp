#include "odf/app/cli_options.hpp"

#include <charconv>
#include <cmath>
#include <limits>

namespace odf::app {
namespace {

Result<std::string> nextValue(const std::vector<std::string>& arguments,
                              std::size_t& index) {
    if (index + 1U >= arguments.size()) {
        return Status::error(ErrorCode::InvalidArgument,
                             "missing value after " + arguments[index]);
    }
    ++index;
    return arguments[index];
}

Result<int> parseInteger(const std::string& value, const std::string& option) {
    int result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) {
        return Status::error(ErrorCode::InvalidArgument,
                             option + " requires an integer value");
    }
    return result;
}

Result<float> parseFloat(const std::string& value, const std::string& option) {
    try {
        std::size_t consumed = 0;
        const float result = std::stof(value, &consumed);
        if (consumed != value.size() || !std::isfinite(result)) {
            throw std::invalid_argument("not finite");
        }
        return result;
    } catch (...) {
        return Status::error(ErrorCode::InvalidArgument,
                             option + " requires a finite numeric value");
    }
}

}  // namespace

Result<CommandLineOptions> parseCommandLine(const std::vector<std::string>& arguments) {
    CommandLineOptions result;
    for (std::size_t index = 1; index < arguments.size(); ++index) {
        const auto& option = arguments[index];
        if (option == "--help" || option == "-h") {
            result.help = true;
            continue;
        }
        if (option == "--allow-unvalidated-model") {
            result.allowUnvalidatedModel = true;
            continue;
        }
        if (option == "--odf-smoke-test") {
            result.smokeTest = true;
            continue;
        }
        const bool expectsValue =
            option == "--models" || option == "--model" || option == "--backend" ||
            option == "--image" || option == "--output" || option == "--log-level" ||
            option == "--camera" || option == "--confidence" || option == "--iou" ||
            option == "--max-detections";
        if (!expectsValue) {
            return Status::error(ErrorCode::InvalidArgument,
                                 "unknown command-line option: " + option);
        }
        auto value = nextValue(arguments, index);
        if (!value.ok()) return value.status();
        if (option == "--models") result.models = value.value();
        else if (option == "--model") {
            result.model = value.value();
            result.modelSpecified = true;
        } else if (option == "--backend") {
            result.backend = value.value();
            result.backendSpecified = true;
        }
        else if (option == "--image") result.image = value.value();
        else if (option == "--output") result.output = value.value();
        else if (option == "--log-level") result.logLevel = value.value();
        else if (option == "--camera") {
            auto parsed = parseInteger(value.value(), option);
            if (!parsed.ok() || parsed.value() < 0 || parsed.value() > 255) {
                return Status::error(ErrorCode::InvalidArgument,
                                     "--camera must be an integer in [0,255]");
            }
            result.camera = parsed.value();
        } else if (option == "--confidence" || option == "--iou") {
            auto parsed = parseFloat(value.value(), option);
            if (!parsed.ok() || parsed.value() < 0.0F || parsed.value() > 1.0F) {
                return Status::error(ErrorCode::InvalidArgument,
                                     option + " must be in [0,1]");
            }
            if (option == "--confidence") {
                result.confidence = parsed.value();
                result.confidenceSpecified = true;
            } else {
                result.iou = parsed.value();
                result.iouSpecified = true;
            }
        } else if (option == "--max-detections") {
            auto parsed = parseInteger(value.value(), option);
            if (!parsed.ok() || parsed.value() < 1 || parsed.value() > 100000) {
                return Status::error(ErrorCode::InvalidArgument,
                                     "--max-detections must be in [1,100000]");
            }
            result.maxDetections = static_cast<std::size_t>(parsed.value());
            result.maxDetectionsSpecified = true;
        }
    }
    if (result.image && result.camera) {
        return Status::error(ErrorCode::InvalidArgument,
                             "--image and --camera are mutually exclusive");
    }
    return result;
}

std::string commandLineUsage(const std::string& executableName) {
    return "Usage: " + executableName +
           " [--models PATH] [--model ID] [--backend ID] [--image PATH | --camera N] "
           "[--output PATH] [--confidence N] [--iou N] [--max-detections N] "
           "[--log-level LEVEL] [--allow-unvalidated-model]";
}

}  // namespace odf::app
