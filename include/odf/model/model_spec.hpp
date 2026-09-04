#pragma once

#include "odf/tensor.hpp"

#include <array>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace odf::model {

enum class ModelFamily { NanoDet, PicoDet, Yolo26 };
enum class SourceFramework { PyTorch, PaddlePaddle };
enum class ColorOrder { Bgr, Rgb };
enum class TensorLayout { Chw };
enum class ResizeMode { Direct, Letterbox };
enum class Precision { Fp32, Fp16, Int8 };

struct InputSpec {
    std::string name{"images"};
    int width{0};
    int height{0};
    ColorOrder colorOrder{ColorOrder::Rgb};
    TensorLayout layout{TensorLayout::Chw};
    DataType dataType{DataType::Float32};
    std::vector<std::string> auxiliaryInputs;
};

struct PreprocessConfig {
    ResizeMode resize{ResizeMode::Letterbox};
    float padValue{0.0F};
    float scale{1.0F};
    std::array<float, 3> mean{0.0F, 0.0F, 0.0F};
    std::array<float, 3> standardDeviation{1.0F, 1.0F, 1.0F};
};

struct PostprocessConfig {
    std::string mode;
    bool requiresNms{true};
    float confidenceThreshold{0.25F};
    float iouThreshold{0.45F};
    int regressionMax{7};
    std::vector<int> strides;
};

struct ModelArtifact {
    std::string backend;
    std::map<std::string, std::filesystem::path> files;
    std::vector<std::string> inputNames;
    std::vector<std::string> outputNames;
};

struct ModelSpec {
    int schemaVersion{1};
    std::string id;
    std::string displayName;
    ModelFamily family{ModelFamily::NanoDet};
    SourceFramework sourceFramework{SourceFramework::PyTorch};
    std::string variant;
    InputSpec input;
    PreprocessConfig preprocess;
    PostprocessConfig postprocess;
    int classCount{0};
    std::vector<std::string> labels;
    std::vector<std::string> supportedBackends;
    std::string defaultBackend;
    std::map<std::string, ModelArtifact> artifacts;
    std::vector<Precision> precisions{Precision::Fp32};
    bool deploymentValidated{false};
    std::filesystem::path metadataPath;
};

const char* modelFamilyName(ModelFamily family) noexcept;
Result<ModelFamily> parseModelFamily(const std::string& value);
Result<SourceFramework> parseSourceFramework(const std::string& value);

}  // namespace odf::model
