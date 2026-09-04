#include "odf/benchmark/statistics.hpp"
#include "odf/benchmark/performance_benchmark.hpp"
#include "odf/camera/latest_frame_buffer.hpp"
#include "odf/detection/nms.hpp"
#include "odf/image/preprocess.hpp"
#include "odf/model/model_registry.hpp"
#include "odf/models/nanodet_adapter.hpp"
#include "odf/models/picodet_adapter.hpp"
#include "odf/models/register_adapters.hpp"
#include "odf/models/yolo26_adapter.hpp"
#include "odf/pipeline/detector_factory.hpp"
#include "odf/pipeline/detector_pipeline.hpp"

#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using odf::detection::BoundingBox;
using odf::detection::Detection;

void expect(bool condition, const char* expression, int line) {
    if (!condition) {
        throw std::runtime_error(std::string("line ") + std::to_string(line) +
                                 ": expectation failed: " + expression);
    }
}

#define EXPECT_TRUE(value) expect((value), #value, __LINE__)
#define EXPECT_NEAR(lhs, rhs, tolerance) \
    expect(std::fabs((lhs) - (rhs)) <= (tolerance), #lhs " ~= " #rhs, __LINE__)

odf::model::ModelSpec baseSpec(odf::model::ModelFamily family, std::string mode,
                               bool requiresNms, int classes = 2) {
    odf::model::ModelSpec spec;
    spec.id = "fixture";
    spec.displayName = "Fixture";
    spec.variant = "test";
    spec.family = family;
    spec.sourceFramework = family == odf::model::ModelFamily::PicoDet
                               ? odf::model::SourceFramework::PaddlePaddle
                               : odf::model::SourceFramework::PyTorch;
    spec.input.name = "images";
    spec.input.width = 8;
    spec.input.height = 8;
    spec.input.colorOrder = odf::model::ColorOrder::Rgb;
    spec.preprocess.resize = odf::model::ResizeMode::Direct;
    spec.classCount = classes;
    for (int index = 0; index < classes; ++index) {
        spec.labels.push_back("class-" + std::to_string(index));
    }
    spec.supportedBackends = {"mock"};
    spec.defaultBackend = "mock";
    spec.postprocess.mode = std::move(mode);
    spec.postprocess.requiresNms = requiresNms;
    spec.postprocess.regressionMax = 7;
    spec.postprocess.strides = {8};
    spec.artifacts.emplace(
        "mock", odf::model::ModelArtifact{"mock", {{"model", "fixture"}},
                                           {"images"}, {"output"}});
    spec.deploymentValidated = true;
    return spec;
}

odf::image::PreprocessTransform identityTransform(int width = 8, int height = 8) {
    return {width, height, width, height, 1.0F, 1.0F, 0.0F, 0.0F, false};
}

void testBoundingBoxAndNms() {
    const BoundingBox first{0, 0, 10, 10};
    const BoundingBox second{5, 5, 15, 15};
    EXPECT_NEAR(first.width(), 10.0F, 1.0e-6F);
    EXPECT_NEAR(first.area(), 100.0F, 1.0e-6F);
    EXPECT_TRUE(first.valid());
    EXPECT_NEAR(odf::detection::intersectionOverUnion(first, second), 25.0F / 175.0F,
                1.0e-6F);
    const BoundingBox invalid{1, 1, 1, 2};
    EXPECT_TRUE(!invalid.valid());

    const std::vector<Detection> input{{{0, 0, 10, 10}, 0, 0.9F},
                                       {{1, 1, 11, 11}, 0, 0.8F},
                                       {{1, 1, 11, 11}, 1, 0.7F}};
    auto aware = odf::detection::nonMaximumSuppression(input, 0.5F, 10, false);
    EXPECT_TRUE(aware.ok() && aware.value().size() == 2U);
    auto agnostic = odf::detection::nonMaximumSuppression(input, 0.5F, 10, true);
    EXPECT_TRUE(agnostic.ok() && agnostic.value().size() == 1U);
    auto selected = odf::detection::filterSelectedClasses(input, {1});
    EXPECT_TRUE(selected.size() == 1U && selected[0].classId == 1);
}

void testPreprocessingAndRestore() {
    odf::image::Image image;
    image.width = 2;
    image.height = 1;
    image.format = odf::image::PixelFormat::Bgr8;
    image.pixels = {10, 20, 30, 40, 50, 60};
    odf::model::InputSpec input;
    input.width = 2;
    input.height = 1;
    input.colorOrder = odf::model::ColorOrder::Rgb;
    odf::model::PreprocessConfig config;
    auto result = odf::image::preprocessImage(image, input, config);
    EXPECT_TRUE(result.ok());
    EXPECT_NEAR(result.value().tensors[0].data[0], 30.0F, 1.0e-5F);
    EXPECT_NEAR(result.value().tensors[0].data[2], 20.0F, 1.0e-5F);
    EXPECT_NEAR(result.value().tensors[0].data[4], 10.0F, 1.0e-5F);

    odf::image::Image wide;
    wide.width = 4;
    wide.height = 2;
    wide.pixels.assign(24, 0);
    input.width = 8;
    input.height = 8;
    config.resize = odf::model::ResizeMode::Letterbox;
    auto letterbox = odf::image::preprocessImage(wide, input, config);
    EXPECT_TRUE(letterbox.ok());
    EXPECT_NEAR(letterbox.value().transform.padY, 2.0F, 1.0e-5F);
    const auto restored = odf::image::restoreBox({0, 2, 8, 6},
                                                 letterbox.value().transform);
    EXPECT_NEAR(restored.x2, 4.0F, 1.0e-5F);
    EXPECT_NEAR(restored.y2, 2.0F, 1.0e-5F);
}

void testRegistry() {
    odf::model::ModelRegistry registry;
    const auto status = registry.loadDirectory(std::filesystem::path(ODF_SOURCE_DIR) / "models");
    EXPECT_TRUE(status.ok());
    EXPECT_TRUE(registry.size() == 16U);
    EXPECT_TRUE(registry.find("yolo26x") != nullptr);
    EXPECT_TRUE(registry.find("picodet-l-640") != nullptr);
    auto invalidFile = odf::model::loadModelSpec(
        std::filesystem::path(ODF_SOURCE_DIR) / "tests" / "fixtures" / "invalid_model.json");
    EXPECT_TRUE(!invalidFile.ok());
    auto invalid = baseSpec(odf::model::ModelFamily::Yolo26, "yolo26_end2end", true);
    EXPECT_TRUE(!odf::model::validateModelSpec(invalid).ok());
}

void testYoloParsers() {
    odf::models::Yolo26Adapter adapter;
    auto endSpec = baseSpec(odf::model::ModelFamily::Yolo26, "yolo26_end2end", false);
    odf::Tensor endOutput{"output", {1, 2, 6}, odf::DataType::Float32,
                          {1, 1, 5, 5, 0.9F, 1, 0, 0, 3, 3, 0.1F, 0}};
    odf::detection::InferenceOptions options;
    options.confidenceThreshold = 0.5F;
    auto endResult = adapter.postprocess({endOutput}, identityTransform(), endSpec, options);
    EXPECT_TRUE(endResult.ok() && endResult.value().size() == 1U);
    EXPECT_TRUE(endResult.value()[0].classId == 1);

    auto traditional = baseSpec(odf::model::ModelFamily::Yolo26,
                                "yolo26_one_to_many", true);
    odf::Tensor raw{"output", {1, 6, 2}, odf::DataType::Float32,
                    {4, 4, 4, 4, 4, 4, 4, 4, 0.9F, 0.8F, 0.1F, 0.2F}};
    auto rawResult = adapter.postprocess({raw}, identityTransform(), traditional, options);
    EXPECT_TRUE(rawResult.ok() && rawResult.value().size() == 1U);
}

void testNanoDetParser() {
    odf::models::NanoDetAdapter adapter;
    auto spec = baseSpec(odf::model::ModelFamily::NanoDet, "nanodet_plus_dfl", true);
    const int channels = 2 + 4 * 8;
    odf::Tensor output;
    output.name = "output";
    output.shape = {1, 1, channels};
    output.data.assign(static_cast<std::size_t>(channels), -10.0F);
    output.data[0] = 0.9F;
    output.data[1] = 0.1F;
    for (int side = 0; side < 4; ++side) output.data[2 + side * 8 + 1] = 10.0F;
    odf::detection::InferenceOptions options;
    options.confidenceThreshold = 0.5F;
    auto result = adapter.postprocess({output}, identityTransform(), spec, options);
    EXPECT_TRUE(result.ok() && result.value().size() == 1U);
}

void testPicoDetParser() {
    odf::models::PicoDetAdapter adapter;
    auto spec = baseSpec(odf::model::ModelFamily::PicoDet,
                         "picodet_paddle_bbox_original", false);
    odf::Tensor boxes{"bbox", {2, 6}, odf::DataType::Float32,
                      {1, 0.8F, 1, 1, 4, 4, 0, 0.1F, 0, 0, 2, 2}};
    odf::Tensor count{"bbox_num", {1}, odf::DataType::Float32, {2}};
    odf::detection::InferenceOptions options;
    options.confidenceThreshold = 0.5F;
    auto result = adapter.postprocess({boxes, count}, identityTransform(), spec, options);
    EXPECT_TRUE(result.ok() && result.value().size() == 1U);
}

class MockBackend final : public odf::backend::IInferenceBackend {
public:
    explicit MockBackend(odf::Tensor output) : output_(std::move(output)) {}
    odf::backend::BackendInfo info() const override {
        return {"mock", "test", true, {}, {{"CPU", "CPU"}},
                {odf::model::Precision::Fp32}, {"fixture"}, false, false, false};
    }
    odf::Status load(const odf::model::ModelArtifact&,
                     const odf::backend::BackendConfig&) override {
        loaded_ = true;
        return odf::Status::success();
    }
    odf::Result<std::vector<odf::Tensor>> infer(
        const std::vector<odf::Tensor>&) override {
        if (!loaded_) {
            return odf::Status::error(odf::ErrorCode::InferenceFailure, "not loaded");
        }
        return std::vector<odf::Tensor>{output_};
    }
    void unload() noexcept override { loaded_ = false; }

private:
    odf::Tensor output_;
    bool loaded_{false};
};

void testFactoryPipelineAndStaleResults() {
    odf::pipeline::DetectorFactory factory;
    EXPECT_TRUE(odf::models::registerTorchAdapters(factory).ok());
    const odf::Tensor output{"output", {1, 1, 6}, odf::DataType::Float32,
                             {1, 1, 5, 5, 0.9F, 0}};
    EXPECT_TRUE(factory.registerBackend("mock", [output] {
        return std::make_unique<MockBackend>(output);
    }).ok());
    auto spec = baseSpec(odf::model::ModelFamily::Yolo26, "yolo26_end2end", false);
    auto detector = factory.create(spec, "mock");
    EXPECT_TRUE(detector.ok());
    EXPECT_TRUE(detector.value()->load(spec, {}).ok());
    odf::image::ImageFrame frame;
    frame.frameId = 7;
    frame.image.width = 8;
    frame.image.height = 8;
    frame.image.pixels.assign(8U * 8U * 3U, 0);
    auto detected = detector.value()->detect(frame, {});
    EXPECT_TRUE(detected.ok() && detected.value().detections.size() == 1U);
    EXPECT_TRUE(odf::pipeline::isCurrentResult(detected.value(), 7, 1, "fixture"));
    EXPECT_TRUE(!odf::pipeline::isCurrentResult(detected.value(), 8, 1, "fixture"));
}

void testStatistics() {
    auto statistics = odf::benchmark::calculateStatistics({1, 2, 3, 4, 5});
    EXPECT_TRUE(statistics.ok());
    EXPECT_NEAR(statistics.value().mean, 3.0, 1.0e-9);
    EXPECT_NEAR(statistics.value().p50, 3.0, 1.0e-9);
    EXPECT_TRUE(!odf::benchmark::calculateStatistics({}).ok());
}

void testPerformanceBenchmark() {
    const odf::Tensor output{"output", {1, 1, 6}, odf::DataType::Float32,
                             {1, 1, 5, 5, 0.9F, 0}};
    odf::pipeline::DetectorPipeline detector(
        std::make_unique<odf::models::Yolo26Adapter>(),
        std::make_unique<MockBackend>(output));
    auto spec = baseSpec(odf::model::ModelFamily::Yolo26, "yolo26_end2end", false);
    EXPECT_TRUE(detector.load(spec, {}).ok());
    odf::image::ImageFrame frame;
    frame.image.width = 8;
    frame.image.height = 8;
    frame.image.pixels.assign(8U * 8U * 3U, 0);
    odf::benchmark::PerformanceBenchmarkConfig config;
    config.warmupCount = 2;
    config.measuredIterations = 3;
    config.backend = "mock";
    auto result = odf::benchmark::runPerformanceBenchmark(detector, frame, config);
    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.value().measuredIterations == 3U);
    EXPECT_TRUE(result.value().totalMs.mean >= 0.0);
}

void testLatestFrameBuffer() {
    odf::camera::LatestFrameBuffer buffer;
    odf::image::ImageFrame first;
    first.frameId = 1;
    first.image.width = 1;
    first.image.height = 1;
    first.image.pixels = {0, 0, 0};
    auto second = first;
    second.frameId = 2;
    EXPECT_TRUE(buffer.push(std::move(first)).ok());
    EXPECT_TRUE(buffer.push(std::move(second)).ok());
    auto received = buffer.waitAndTake();
    EXPECT_TRUE(received.ok() && received.value().frameId == 2U);
    const auto statistics = buffer.statistics();
    EXPECT_TRUE(statistics.captured == 2U && statistics.consumed == 1U &&
                statistics.dropped == 1U);
    buffer.close();
    EXPECT_TRUE(!buffer.waitAndTake().ok());
}

}  // namespace

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests{
        {"BoundingBox/NMS/class filter", testBoundingBoxAndNms},
        {"preprocess/letterbox/restore", testPreprocessingAndRestore},
        {"model registry", testRegistry},
        {"YOLO26 parsers", testYoloParsers},
        {"NanoDet parser", testNanoDetParser},
        {"PicoDet parser", testPicoDetParser},
        {"factory/pipeline/stale result", testFactoryPipelineAndStaleResults},
        {"benchmark statistics", testStatistics},
        {"performance benchmark", testPerformanceBenchmark},
        {"latest-frame camera buffer", testLatestFrameBuffer},
    };
    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.second();
            std::cout << "[PASS] " << test.first << '\n';
        } catch (const std::exception& exception) {
            ++failures;
            std::cerr << "[FAIL] " << test.first << ": " << exception.what() << '\n';
        }
    }
    std::cout << tests.size() - static_cast<std::size_t>(failures) << '/'
              << tests.size() << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
