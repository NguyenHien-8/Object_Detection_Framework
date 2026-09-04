#include "odf/app/cli_options.hpp"
#include "odf/app/model_root.hpp"
#include "odf/app/logging_options.hpp"
#include "odf/app/opencv_bridge.hpp"
#include "odf/app/runtime_catalog.hpp"
#include "odf/camera/latest_frame_buffer.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char** argv) {
    std::vector<std::string> arguments(argv, argv + argc);
    auto options = odf::app::parseCommandLine(arguments);
    if (!options.ok() || (!options.value().camera && !options.value().help)) {
        if (!options.ok()) std::cerr << options.status().message() << '\n';
        std::cout << odf::app::commandLineUsage(arguments[0]) << '\n';
        return options.ok() && options.value().help ? 0 : 2;
    }
    if (options.value().help) {
        std::cout << odf::app::commandLineUsage(arguments[0]) << '\n';
        return 0;
    }
    const auto logStatus = odf::app::applyLogLevel(options.value().logLevel);
    if (!logStatus.ok()) {
        std::cerr << logStatus.message() << '\n';
        return 2;
    }
    auto modelRoot = odf::app::discoverModelRoot(
        {options.value().models, odf::app::modelRootFromEnvironment(),
         std::filesystem::absolute(arguments[0]).parent_path(),
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
    auto capture = std::make_shared<cv::VideoCapture>(*options.value().camera);
    if (!capture->isOpened()) {
        std::cerr << "camera " << *options.value().camera << " could not be opened\n";
        return 1;
    }
    odf::camera::LatestFrameBuffer buffer;
    std::atomic_bool stop{false};
    std::thread captureThread([capture, &buffer, &stop] {
        std::uint64_t frameId = 0;
        cv::Mat frame;
        while (!stop.load(std::memory_order_relaxed)) {
            if (!capture->read(frame) || frame.empty()) break;
            auto image = odf::app::toOdfImage(frame);
            if (!image.ok()) break;
            odf::image::ImageFrame item{image.takeValue(), std::chrono::steady_clock::now(),
                                        ++frameId, "camera"};
            if (!buffer.push(std::move(item)).ok()) break;
        }
        buffer.close();
    });

    odf::detection::InferenceOptions inferenceOptions;
    inferenceOptions.confidenceThreshold = options.value().confidence;
    inferenceOptions.iouThreshold = options.value().iou;
    inferenceOptions.maxDetections = options.value().maxDetections;
    while (true) {
        auto frame = buffer.waitAndTake();
        if (!frame.ok()) break;
        auto result = detector.value()->detect(frame.value(), inferenceOptions);
        if (!result.ok()) {
            std::cerr << result.status().message() << '\n';
            break;
        }
        auto source = odf::app::toCvImage(frame.value().image);
        const auto model = detector.value()->modelInfo();
        if (!source.ok() || !model) {
            std::cerr << (source.ok() ? "loaded detector did not expose model metadata"
                                      : source.status().message())
                      << '\n';
            break;
        }
        auto rendered = odf::app::renderDetections(source.value(), result.value().detections,
                                                   model->labels);
        if (!rendered.ok()) {
            std::cerr << rendered.status().message() << '\n';
            break;
        }
        if (options.value().smokeTest) {
            std::cout << "camera_frame=" << frame.value().frameId
                      << " detections=" << result.value().detections.size()
                      << " total_ms=" << result.value().timings.totalMs << '\n';
            break;
        }
        const auto statistics = buffer.statistics();
        cv::putText(rendered.value(),
                    "total " + cv::format("%.1f ms", result.value().timings.totalMs) +
                        " dropped " + std::to_string(statistics.dropped),
                    {10, 24}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0, 255, 0}, 2,
                    cv::LINE_AA);
        cv::imshow("ODF Camera Detection - ESC/Q to stop", rendered.value());
        const int key = cv::waitKey(1);
        if (key == 27 || key == 'q' || key == 'Q') break;
    }
    stop.store(true, std::memory_order_relaxed);
    buffer.close();
    if (captureThread.joinable()) captureThread.join();
    capture->release();
    cv::destroyAllWindows();
    return 0;
}
