#include "odf/model/model_registry.hpp"

#include "model_registry/json.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>

namespace odf::model {
namespace {

using internal::JsonValue;

Result<const JsonValue*> required(const JsonValue& parent,
                                  const std::string& key,
                                  JsonValue::Type type) {
    const JsonValue* value = parent.find(key);
    if (value == nullptr) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "missing required model metadata key='" + key + "'");
    }
    if (value->type != type) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model metadata key='" + key + "' has the wrong JSON type");
    }
    return value;
}

Result<std::string> stringValue(const JsonValue& parent, const std::string& key) {
    auto value = required(parent, key, JsonValue::Type::String);
    return value.ok() ? Result<std::string>(value.value()->string)
                      : Result<std::string>(value.status());
}

Result<int> intValue(const JsonValue& parent, const std::string& key) {
    auto value = required(parent, key, JsonValue::Type::Number);
    if (!value.ok()) {
        return value.status();
    }
    const double number = value.value()->number;
    if (std::floor(number) != number || number < std::numeric_limits<int>::min() ||
        number > std::numeric_limits<int>::max()) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model metadata key='" + key + "' must be an integer");
    }
    return static_cast<int>(number);
}

Result<float> floatValue(const JsonValue& parent, const std::string& key) {
    auto value = required(parent, key, JsonValue::Type::Number);
    if (!value.ok()) {
        return value.status();
    }
    return static_cast<float>(value.value()->number);
}

Result<bool> boolValue(const JsonValue& parent, const std::string& key) {
    auto value = required(parent, key, JsonValue::Type::Boolean);
    return value.ok() ? Result<bool>(value.value()->boolean) : Result<bool>(value.status());
}

Result<std::vector<std::string>> stringArray(const JsonValue& parent,
                                             const std::string& key) {
    auto value = required(parent, key, JsonValue::Type::Array);
    if (!value.ok()) {
        return value.status();
    }
    std::vector<std::string> result;
    result.reserve(value.value()->array.size());
    for (const auto& entry : value.value()->array) {
        if (entry.type != JsonValue::Type::String || entry.string.empty()) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "model metadata key='" + key +
                                     "' must contain non-empty strings");
        }
        result.push_back(entry.string);
    }
    return result;
}

Result<std::vector<int>> intArray(const JsonValue& parent, const std::string& key) {
    auto value = required(parent, key, JsonValue::Type::Array);
    if (!value.ok()) {
        return value.status();
    }
    std::vector<int> result;
    result.reserve(value.value()->array.size());
    for (const auto& entry : value.value()->array) {
        if (entry.type != JsonValue::Type::Number || std::floor(entry.number) != entry.number ||
            entry.number < std::numeric_limits<int>::min() ||
            entry.number > std::numeric_limits<int>::max()) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "model metadata key='" + key + "' must contain integers");
        }
        result.push_back(static_cast<int>(entry.number));
    }
    return result;
}

Result<std::array<float, 3>> floatTriplet(const JsonValue& parent,
                                         const std::string& key) {
    auto value = required(parent, key, JsonValue::Type::Array);
    if (!value.ok()) {
        return value.status();
    }
    if (value.value()->array.size() != 3U) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model metadata key='" + key + "' must contain three numbers");
    }
    std::array<float, 3> result{};
    for (std::size_t index = 0; index < result.size(); ++index) {
        if (value.value()->array[index].type != JsonValue::Type::Number) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "model metadata key='" + key + "' must contain numbers");
        }
        result[index] = static_cast<float>(value.value()->array[index].number);
    }
    return result;
}

Status readLabels(const std::filesystem::path& path, std::vector<std::string>& labels) {
    std::ifstream stream(path);
    if (!stream) {
        return Status::error(ErrorCode::FileNotFound,
                             "label file not found: " + path.string());
    }
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) continue;
        labels.push_back(std::move(line));
    }
    return Status::success();
}

}  // namespace

const char* modelFamilyName(ModelFamily family) noexcept {
    switch (family) {
        case ModelFamily::NanoDet: return "nanodet";
        case ModelFamily::PicoDet: return "picodet";
        case ModelFamily::Yolo26: return "yolo26";
    }
    return "unknown";
}

Result<ModelFamily> parseModelFamily(const std::string& value) {
    if (value == "nanodet") return ModelFamily::NanoDet;
    if (value == "picodet") return ModelFamily::PicoDet;
    if (value == "yolo26") return ModelFamily::Yolo26;
    return Status::error(ErrorCode::InvalidModelConfig,
                         "unsupported model family='" + value + "'");
}

Result<SourceFramework> parseSourceFramework(const std::string& value) {
    if (value == "pytorch") return SourceFramework::PyTorch;
    if (value == "paddlepaddle") return SourceFramework::PaddlePaddle;
    return Status::error(ErrorCode::InvalidModelConfig,
                         "unsupported source_framework='" + value + "'");
}

Status validateModelSpec(const ModelSpec& spec) {
    if (spec.schemaVersion != 1) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "unsupported model schema_version=" +
                                 std::to_string(spec.schemaVersion));
    }
    if (spec.id.empty() || spec.displayName.empty() || spec.variant.empty()) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model id, display name, and variant must be non-empty");
    }
    if (!std::all_of(spec.id.begin(), spec.id.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' || character == '_' ||
                   character == '.';
        })) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model id contains unsupported characters");
    }
    if ((spec.family == ModelFamily::PicoDet) !=
        (spec.sourceFramework == SourceFramework::PaddlePaddle)) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model family and source framework are inconsistent");
    }
    if (spec.input.width <= 0 || spec.input.height <= 0 || spec.input.width > 16384 ||
        spec.input.height > 16384) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model input dimensions must be in [1, 16384]");
    }
    if (spec.classCount <= 0 || spec.classCount > 100000 ||
        spec.labels.size() != static_cast<std::size_t>(spec.classCount)) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "class_count must be positive and match the label count");
    }
    if (std::any_of(spec.labels.begin(), spec.labels.end(),
                    [](const std::string& label) { return label.empty(); })) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model labels must not be empty");
    }
    if (spec.supportedBackends.empty() || spec.defaultBackend.empty() ||
        std::find(spec.supportedBackends.begin(), spec.supportedBackends.end(),
                  spec.defaultBackend) == spec.supportedBackends.end()) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "default_backend must appear in supported_backends");
    }
    std::set<std::string> uniqueBackends;
    for (const auto& backend : spec.supportedBackends) {
        if (backend.empty() || !uniqueBackends.insert(backend).second) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "supported_backends must contain unique non-empty names");
        }
        const auto artifact = spec.artifacts.find(backend);
        if (artifact == spec.artifacts.end() || artifact->second.backend != backend ||
            artifact->second.files.empty() || artifact->second.inputNames.empty() ||
            artifact->second.outputNames.empty()) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "every supported backend requires a complete artifact mapping");
        }
    }
    if (!std::isfinite(spec.postprocess.confidenceThreshold) ||
        spec.postprocess.confidenceThreshold < 0.0F ||
        spec.postprocess.confidenceThreshold > 1.0F ||
        !std::isfinite(spec.postprocess.iouThreshold) ||
        spec.postprocess.iouThreshold < 0.0F || spec.postprocess.iouThreshold > 1.0F) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model thresholds must be finite and in [0, 1]");
    }
    if (!std::isfinite(spec.preprocess.scale) || !std::isfinite(spec.preprocess.padValue) ||
        std::any_of(spec.preprocess.mean.begin(), spec.preprocess.mean.end(),
                    [](float value) { return !std::isfinite(value); }) ||
        std::any_of(spec.preprocess.standardDeviation.begin(),
                    spec.preprocess.standardDeviation.end(),
                    [](float value) { return !std::isfinite(value) || value == 0.0F; })) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "preprocess numbers must be finite and std must be non-zero");
    }
    if (spec.postprocess.regressionMax < 1 || spec.postprocess.regressionMax > 64 ||
        spec.postprocess.strides.empty() ||
        std::any_of(spec.postprocess.strides.begin(), spec.postprocess.strides.end(),
                    [](int stride) { return stride <= 0; })) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "postprocess reg_max/strides are outside safe ranges");
    }
    const bool validMode =
        (spec.family == ModelFamily::NanoDet &&
         spec.postprocess.mode == "nanodet_plus_dfl" && spec.postprocess.requiresNms) ||
        (spec.family == ModelFamily::PicoDet &&
         spec.postprocess.mode == "picodet_paddle_bbox_original" &&
         !spec.postprocess.requiresNms) ||
        (spec.family == ModelFamily::Yolo26 &&
         ((spec.postprocess.mode == "yolo26_end2end" &&
           !spec.postprocess.requiresNms) ||
          (spec.postprocess.mode == "yolo26_one_to_many" &&
           spec.postprocess.requiresNms)));
    if (!validMode) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "postprocess mode/NMS setting is inconsistent with the model family");
    }
    return Status::success();
}

Result<ModelSpec> loadModelSpec(const std::filesystem::path& metadataFile) {
    std::error_code error;
    const auto size = std::filesystem::file_size(metadataFile, error);
    if (error) {
        return Status::error(ErrorCode::FileNotFound,
                             "model metadata not found: " + metadataFile.string());
    }
    if (size > 4U * 1024U * 1024U) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model metadata exceeds the 4 MiB safety limit");
    }
    std::ifstream stream(metadataFile, std::ios::binary);
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    auto rootResult = internal::parseJson(buffer.str());
    if (!rootResult.ok()) return rootResult.status();
    const auto& root = rootResult.value();
    if (root.type != JsonValue::Type::Object) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model metadata root must be an object");
    }

    ModelSpec spec;
    auto schema = intValue(root, "schema_version");
    auto id = stringValue(root, "id");
    auto displayName = stringValue(root, "display_name");
    auto familyText = stringValue(root, "family");
    auto frameworkText = stringValue(root, "source_framework");
    auto variant = stringValue(root, "variant");
    auto classCount = intValue(root, "class_count");
    auto backends = stringArray(root, "supported_backends");
    auto defaultBackend = stringValue(root, "default_backend");
    auto deploymentValidated = boolValue(root, "deployment_validated");
    if (!schema.ok()) return schema.status();
    if (!id.ok()) return id.status();
    if (!displayName.ok()) return displayName.status();
    if (!familyText.ok()) return familyText.status();
    if (!frameworkText.ok()) return frameworkText.status();
    if (!variant.ok()) return variant.status();
    if (!classCount.ok()) return classCount.status();
    if (!backends.ok()) return backends.status();
    if (!defaultBackend.ok()) return defaultBackend.status();
    if (!deploymentValidated.ok()) return deploymentValidated.status();
    auto family = parseModelFamily(familyText.value());
    auto framework = parseSourceFramework(frameworkText.value());
    if (!family.ok()) return family.status();
    if (!framework.ok()) return framework.status();

    spec.schemaVersion = schema.value();
    spec.id = id.takeValue();
    spec.displayName = displayName.takeValue();
    spec.family = family.value();
    spec.sourceFramework = framework.value();
    spec.variant = variant.takeValue();
    spec.classCount = classCount.value();
    spec.supportedBackends = backends.takeValue();
    spec.defaultBackend = defaultBackend.takeValue();
    spec.deploymentValidated = deploymentValidated.value();
    spec.metadataPath = std::filesystem::absolute(metadataFile);

    auto inputNode = required(root, "input", JsonValue::Type::Object);
    if (!inputNode.ok()) return inputNode.status();
    auto inputName = stringValue(*inputNode.value(), "name");
    auto width = intValue(*inputNode.value(), "width");
    auto height = intValue(*inputNode.value(), "height");
    auto color = stringValue(*inputNode.value(), "color_order");
    auto layout = stringValue(*inputNode.value(), "layout");
    auto dataType = stringValue(*inputNode.value(), "dtype");
    if (!inputName.ok()) return inputName.status();
    if (!width.ok()) return width.status();
    if (!height.ok()) return height.status();
    if (!color.ok()) return color.status();
    if (!layout.ok()) return layout.status();
    if (!dataType.ok()) return dataType.status();
    if ((color.value() != "BGR" && color.value() != "RGB") || layout.value() != "CHW" ||
        dataType.value() != "FP32") {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "ODF 0.1 registry supports BGR/RGB, CHW, FP32 inputs");
    }
    spec.input.name = inputName.takeValue();
    spec.input.width = width.value();
    spec.input.height = height.value();
    spec.input.colorOrder = color.value() == "RGB" ? ColorOrder::Rgb : ColorOrder::Bgr;
    if (const auto* auxiliary = inputNode.value()->find("auxiliary_inputs")) {
        if (auxiliary->type != JsonValue::Type::Array) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "input.auxiliary_inputs must be an array");
        }
        for (const auto& entry : auxiliary->array) {
            if (entry.type != JsonValue::Type::String || entry.string.empty()) {
                return Status::error(ErrorCode::InvalidModelConfig,
                                     "input.auxiliary_inputs must contain non-empty strings");
            }
            spec.input.auxiliaryInputs.push_back(entry.string);
        }
    }

    auto preprocessNode = required(root, "preprocess", JsonValue::Type::Object);
    if (!preprocessNode.ok()) return preprocessNode.status();
    auto resize = stringValue(*preprocessNode.value(), "resize");
    auto padValue = floatValue(*preprocessNode.value(), "pad_value");
    auto scale = floatValue(*preprocessNode.value(), "scale");
    auto mean = floatTriplet(*preprocessNode.value(), "mean");
    auto standardDeviation = floatTriplet(*preprocessNode.value(), "std");
    if (!resize.ok()) return resize.status();
    if (!padValue.ok()) return padValue.status();
    if (!scale.ok()) return scale.status();
    if (!mean.ok()) return mean.status();
    if (!standardDeviation.ok()) return standardDeviation.status();
    if (resize.value() != "direct" && resize.value() != "letterbox") {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "preprocess.resize must be direct or letterbox");
    }
    spec.preprocess.resize = resize.value() == "direct" ? ResizeMode::Direct
                                                        : ResizeMode::Letterbox;
    spec.preprocess.padValue = padValue.value();
    spec.preprocess.scale = scale.value();
    spec.preprocess.mean = mean.value();
    spec.preprocess.standardDeviation = standardDeviation.value();

    auto postprocessNode = required(root, "postprocess", JsonValue::Type::Object);
    if (!postprocessNode.ok()) return postprocessNode.status();
    auto mode = stringValue(*postprocessNode.value(), "mode");
    auto requiresNms = boolValue(*postprocessNode.value(), "requires_nms");
    auto confidence = floatValue(*postprocessNode.value(), "confidence_threshold");
    auto iou = floatValue(*postprocessNode.value(), "iou_threshold");
    auto regressionMax = intValue(*postprocessNode.value(), "reg_max");
    auto strides = intArray(*postprocessNode.value(), "strides");
    if (!mode.ok()) return mode.status();
    if (!requiresNms.ok()) return requiresNms.status();
    if (!confidence.ok()) return confidence.status();
    if (!iou.ok()) return iou.status();
    if (!regressionMax.ok()) return regressionMax.status();
    if (!strides.ok()) return strides.status();
    spec.postprocess.mode = mode.takeValue();
    spec.postprocess.requiresNms = requiresNms.value();
    spec.postprocess.confidenceThreshold = confidence.value();
    spec.postprocess.iouThreshold = iou.value();
    spec.postprocess.regressionMax = regressionMax.value();
    spec.postprocess.strides = strides.takeValue();

    auto labelsPath = stringValue(root, "labels");
    if (!labelsPath.ok()) return labelsPath.status();
    const auto modelDirectory = metadataFile.parent_path();
    const auto labelStatus = readLabels(modelDirectory / labelsPath.value(), spec.labels);
    if (!labelStatus.ok()) return labelStatus;

    auto artifactNode = required(root, "artifacts", JsonValue::Type::Object);
    if (!artifactNode.ok()) return artifactNode.status();
    for (const auto& backendEntry : artifactNode.value()->object) {
        if (backendEntry.second.type != JsonValue::Type::Object) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "artifact backend entry must be an object");
        }
        ModelArtifact artifact;
        artifact.backend = backendEntry.first;
        auto inputs = stringArray(backendEntry.second, "inputs");
        auto outputs = stringArray(backendEntry.second, "outputs");
        auto files = required(backendEntry.second, "files", JsonValue::Type::Object);
        if (!inputs.ok()) return inputs.status();
        if (!outputs.ok()) return outputs.status();
        if (!files.ok()) return files.status();
        artifact.inputNames = inputs.takeValue();
        artifact.outputNames = outputs.takeValue();
        for (const auto& file : files.value()->object) {
            if (file.second.type != JsonValue::Type::String || file.second.string.empty()) {
                return Status::error(ErrorCode::InvalidModelConfig,
                                     "artifact files must map names to relative paths");
            }
            const std::filesystem::path path(file.second.string);
            if (path.is_absolute()) {
                return Status::error(ErrorCode::InvalidModelConfig,
                                     "artifact paths in committed metadata must be relative");
            }
            artifact.files.emplace(file.first, modelDirectory / path);
        }
        spec.artifacts.emplace(artifact.backend, std::move(artifact));
    }

    const auto status = validateModelSpec(spec);
    if (!status.ok()) return status;
    return spec;
}

Status ModelRegistry::loadDirectory(const std::filesystem::path& root) {
    std::error_code error;
    if (!std::filesystem::is_directory(root, error)) {
        return Status::error(ErrorCode::FileNotFound,
                             "model registry directory not found: " + root.string());
    }
    std::vector<std::filesystem::path> metadataFiles;
    std::filesystem::recursive_directory_iterator iterator(
        root, std::filesystem::directory_options::skip_permission_denied, error);
    const std::filesystem::recursive_directory_iterator end;
    while (!error && iterator != end) {
        if (iterator->is_regular_file(error) && iterator->path().filename() == "model.json") {
            metadataFiles.push_back(iterator->path());
        }
        iterator.increment(error);
    }
    if (error) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "failed while scanning model registry: " + error.message());
    }
    std::sort(metadataFiles.begin(), metadataFiles.end());
    if (metadataFiles.empty()) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "model registry contains no model.json files");
    }
    std::map<std::string, ModelSpec> staged;
    for (const auto& path : metadataFiles) {
        auto spec = loadModelSpec(path);
        if (!spec.ok()) {
            return Status::error(spec.status().code(),
                                 path.string() + ": " + spec.status().message());
        }
        auto loadedSpec = spec.takeValue();
        const auto loadedId = loadedSpec.id;
        if (!staged.emplace(loadedId, std::move(loadedSpec)).second) {
            return Status::error(ErrorCode::InvalidModelConfig,
                                 "duplicate model id while loading: " + path.string());
        }
    }
    models_ = std::move(staged);
    return Status::success();
}

Status ModelRegistry::add(ModelSpec spec) {
    const auto status = validateModelSpec(spec);
    if (!status.ok()) return status;
    const auto id = spec.id;
    if (!models_.emplace(id, std::move(spec)).second) {
        return Status::error(ErrorCode::InvalidModelConfig, "duplicate model id");
    }
    return Status::success();
}

const ModelSpec* ModelRegistry::find(const std::string& id) const noexcept {
    const auto iterator = models_.find(id);
    return iterator == models_.end() ? nullptr : &iterator->second;
}

std::vector<ModelSpec> ModelRegistry::models() const {
    std::vector<ModelSpec> result;
    result.reserve(models_.size());
    for (const auto& entry : models_) result.push_back(entry.second);
    return result;
}

}  // namespace odf::model
