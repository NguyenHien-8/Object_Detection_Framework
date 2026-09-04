#include "odf/app/cli_options.hpp"
#include "odf/app/model_root.hpp"
#include "odf/app/logging_options.hpp"
#include "odf/app/opencv_bridge.hpp"
#include "odf/app/runtime_catalog.hpp"

#include <opencv2/core.hpp>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testCli() {
    auto defaults = odf::app::parseCommandLine({"odf"});
    require(defaults.ok() && !defaults.value().modelSpecified &&
                !defaults.value().confidenceSpecified,
            "default command line was incorrectly marked as explicit");
    auto parsed = odf::app::parseCommandLine(
        {"odf", "--models", "models", "--model", "yolo26n", "--backend",
         "onnxruntime", "--image", "input.jpg", "--output", "output.jpg",
         "--confidence", "0.4", "--allow-unvalidated-model", "--odf-smoke-test"});
    require(parsed.ok(), "valid command line was rejected");
    require(parsed.value().model == "yolo26n", "model option was not parsed");
    require(parsed.value().modelSpecified && parsed.value().backendSpecified,
            "model/backend presence was not tracked");
    require(parsed.value().confidenceSpecified, "threshold presence was not tracked");
    require(parsed.value().allowUnvalidatedModel, "developer override was not parsed");
    require(parsed.value().smokeTest, "internal smoke-test option was not parsed");
    require(!odf::app::parseCommandLine({"odf", "--camera", "-1"}).ok(),
            "negative camera index was accepted");
    require(!odf::app::parseCommandLine(
                {"odf", "--image", "x.jpg", "--camera", "0"}).ok(),
            "mutually exclusive sources were accepted");
    require(odf::app::applyLogLevel("warning").ok(), "valid log level was rejected");
    require(!odf::app::applyLogLevel("loud").ok(), "invalid log level was accepted");
    require(odf::app::applyLogLevel("info").ok(), "log level could not be restored");
}

void testModelRoot() {
    const auto sourceModels = std::filesystem::path(ODF_SOURCE_DIR) / "models";
    auto discovered = odf::app::discoverModelRoot(
        {sourceModels, std::filesystem::path("missing-env"),
         std::filesystem::path("missing-app"), std::filesystem::path("missing-install")});
    require(discovered.ok(), "explicit model root was not selected");
    require(std::filesystem::equivalent(discovered.value(), sourceModels),
            "model-root precedence is incorrect");
    auto invalid = odf::app::discoverModelRoot(
        {std::filesystem::path("missing-explicit"), sourceModels,
         std::filesystem::path("missing-app"), sourceModels});
    require(!invalid.ok(), "invalid explicit model root silently fell back");
}

void testOpenCvBridge() {
    cv::Mat backing(4, 5, CV_8UC3, cv::Scalar(10, 20, 30));
    const cv::Mat roi = backing(cv::Rect(1, 1, 3, 2));
    require(!roi.isContinuous(), "fixture must exercise a strided matrix");
    auto image = odf::app::toOdfImage(roi);
    require(image.ok(), "OpenCV to ODF conversion failed");
    require(image.value().width == 3 && image.value().height == 2,
            "converted dimensions are wrong");
    auto roundTrip = odf::app::toCvImage(image.value());
    require(roundTrip.ok() && roundTrip.value().at<cv::Vec3b>(0, 0) == cv::Vec3b(10, 20, 30),
            "OpenCV bridge changed BGR values");
}

void testRuntimeCatalog() {
    odf::app::RuntimeCatalog catalog;
    auto status = catalog.loadRegistry(std::filesystem::path(ODF_SOURCE_DIR) / "models");
    require(status.ok(), "runtime catalog could not load registry");
    const auto backends = catalog.backendInfos();
    require(backends.size() == 1U && backends[0].name == "onnxruntime" &&
                backends[0].available,
            "ONNX Runtime capability was not registered");
}

}  // namespace

int main() {
    try {
        testCli();
        testModelRoot();
        testOpenCvBridge();
        testRuntimeCatalog();
        std::cout << "4/4 application test groups passed\n";
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "application tests failed: " << exception.what() << '\n';
        return 1;
    }
}
