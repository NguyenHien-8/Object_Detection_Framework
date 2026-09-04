#include "odf/app/cli_options.hpp"
#include "odf/app/model_root.hpp"
#include "odf/app/logging_options.hpp"
#include "odf/app/opencv_bridge.hpp"
#include "odf/app/runtime_catalog.hpp"

#include <opencv2/imgcodecs.hpp>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::vector<std::string> arguments(argv, argv + argc);
    auto options = odf::app::parseCommandLine(arguments);
    if (!options.ok()) {
        std::cerr << options.status().message() << '\n'
                  << odf::app::commandLineUsage(arguments[0]) << '\n';
        return 2;
    }
    if (options.value().help || !options.value().image || !options.value().output) {
        std::cout << odf::app::commandLineUsage(arguments[0]) << '\n';
        return options.value().help ? 0 : 2;
    }
    const auto logStatus = odf::app::applyLogLevel(options.value().logLevel);
    if (!logStatus.ok()) {
        std::cerr << logStatus.message() << '\n';
        return 2;
    }
    const auto executableDirectory = std::filesystem::absolute(arguments[0]).parent_path();
    auto modelRoot = odf::app::discoverModelRoot(
        {options.value().models, odf::app::modelRootFromEnvironment(), executableDirectory,
         odf::app::compiledInstalledModelRoot()});
    if (!modelRoot.ok()) {
        std::cerr << modelRoot.status().message() << '\n';
        return 1;
    }

    odf::app::RuntimeCatalog catalog;
    auto status = catalog.loadRegistry(modelRoot.value());
    if (!status.ok()) {
        std::cerr << status.message() << '\n';
        return 1;
    }
    auto detector = catalog.createLoadedDetector(
        options.value().model, options.value().backend, {},
        options.value().allowUnvalidatedModel);
    if (!detector.ok()) {
        std::cerr << detector.status().message() << '\n';
        return 1;
    }
    const cv::Mat original = cv::imread(options.value().image->string(), cv::IMREAD_COLOR);
    auto image = odf::app::toOdfImage(original);
    if (!image.ok()) {
        std::cerr << image.status().message() << '\n';
        return 1;
    }
    odf::image::ImageFrame frame{image.takeValue(), std::chrono::steady_clock::now(), 1,
                                 options.value().image->string()};
    odf::detection::InferenceOptions inferenceOptions;
    inferenceOptions.confidenceThreshold = options.value().confidence;
    inferenceOptions.iouThreshold = options.value().iou;
    inferenceOptions.maxDetections = options.value().maxDetections;
    auto result = detector.value()->detect(frame, inferenceOptions);
    if (!result.ok()) {
        std::cerr << result.status().message() << '\n';
        return 1;
    }
    const auto model = detector.value()->modelInfo();
    if (!model) {
        std::cerr << "loaded detector did not expose model metadata\n";
        return 1;
    }
    auto rendered = odf::app::renderDetections(original, result.value().detections,
                                               model->labels);
    if (!rendered.ok() || !cv::imwrite(options.value().output->string(), rendered.value())) {
        std::cerr << (rendered.ok() ? "failed to write output image"
                                    : rendered.status().message()) << '\n';
        return 1;
    }
    std::cout << "detections=" << result.value().detections.size()
              << " preprocess_ms=" << result.value().timings.preprocessMs
              << " inference_ms=" << result.value().timings.inferenceMs
              << " postprocess_ms=" << result.value().timings.postprocessMs
              << " total_ms=" << result.value().timings.totalMs << '\n';
    for (const auto& detection : result.value().detections) {
        const auto classIndex = static_cast<std::size_t>(detection.classId);
        std::cout << "detection class_id=" << detection.classId
                  << " label=" << model->labels[classIndex]
                  << " confidence=" << detection.confidence
                  << " box=" << detection.box.x1 << ',' << detection.box.y1 << ','
                  << detection.box.x2 << ',' << detection.box.y2 << '\n';
    }
    return 0;
}
