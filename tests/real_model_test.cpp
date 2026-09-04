#include "odf/app/opencv_bridge.hpp"
#include "odf/app/runtime_catalog.hpp"

#include <opencv2/imgcodecs.hpp>

#include <chrono>
#include <iostream>

int main() {
    odf::app::RuntimeCatalog catalog;
    auto status = catalog.loadRegistry(ODF_TEST_MODEL_ROOT);
    if (!status.ok()) {
        std::cerr << status.message() << '\n';
        return 1;
    }
    auto detector = catalog.createLoadedDetector("yolo26n", "onnxruntime", {}, false);
    if (!detector.ok()) {
        std::cerr << detector.status().message() << '\n';
        return 1;
    }
    const auto source = cv::imread(ODF_TEST_IMAGE, cv::IMREAD_COLOR);
    auto image = odf::app::toOdfImage(source);
    if (!image.ok()) {
        std::cerr << image.status().message() << '\n';
        return 1;
    }
    odf::image::ImageFrame frame{image.takeValue(), std::chrono::steady_clock::now(), 1,
                                 ODF_TEST_IMAGE};
    auto result = detector.value()->detect(frame, {});
    if (!result.ok()) {
        std::cerr << result.status().message() << '\n';
        return 1;
    }
    if (result.value().detections.empty()) {
        std::cerr << "real YOLO26n inference returned no detections at confidence 0.25\n";
        return 1;
    }
    for (const auto& detection : result.value().detections) {
        if (!detection.box.valid() || detection.classId < 0 || detection.classId >= 80 ||
            detection.confidence < 0.25F || detection.confidence > 1.0F) {
            std::cerr << "real YOLO26n inference returned an invalid detection\n";
            return 1;
        }
    }
    detector.value()->unload();
    if (detector.value()->isLoaded()) {
        std::cerr << "detector remained loaded after unload\n";
        return 1;
    }
    const auto* spec = catalog.registry().find("yolo26n");
    if (spec == nullptr || !detector.value()->load(*spec, {}).ok()) {
        std::cerr << "real YOLO26n detector could not be reloaded\n";
        return 1;
    }
    auto reloaded = detector.value()->detect(frame, {});
    if (!reloaded.ok() || reloaded.value().detections.size() != result.value().detections.size()) {
        std::cerr << "real YOLO26n reload changed the detection result\n";
        return 1;
    }
    std::cout << "detections=" << result.value().detections.size()
              << " total_ms=" << result.value().timings.totalMs << '\n';
    return 0;
}
