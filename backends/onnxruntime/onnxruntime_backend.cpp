#include "odf/backends/onnxruntime_backend.hpp"

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

namespace odf::backends {

class OnnxRuntimeBackend::Impl {
public:
    Ort::Env environment{ORT_LOGGING_LEVEL_WARNING, "odf"};
    std::unique_ptr<Ort::Session> session;
    std::vector<std::string> inputNames;
    std::vector<std::string> outputNames;
    std::vector<std::vector<std::int64_t>> inputShapes;
};

namespace {

Status validateNames(const std::vector<std::string>& expected,
                     const std::vector<std::string>& actual,
                     const char* kind) {
    for (const auto& name : expected) {
        if (std::find(actual.begin(), actual.end(), name) == actual.end()) {
            return Status::error(ErrorCode::ModelLoadFailure,
                                 std::string("ONNX model is missing declared ") + kind +
                                     " tensor='" + name + "'");
        }
    }
    return Status::success();
}

bool shapeMatches(const std::vector<std::int64_t>& expected,
                  const std::vector<std::int64_t>& actual) {
    if (expected.size() != actual.size()) return false;
    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (expected[index] > 0 && expected[index] != actual[index]) return false;
    }
    return true;
}

std::string formatShape(const std::vector<std::int64_t>& shape) {
    std::ostringstream stream;
    stream << '[';
    for (std::size_t index = 0; index < shape.size(); ++index) {
        if (index != 0) stream << ',';
        stream << shape[index];
    }
    stream << ']';
    return stream.str();
}

}  // namespace

OnnxRuntimeBackend::OnnxRuntimeBackend() : impl_(std::make_unique<Impl>()) {}
OnnxRuntimeBackend::~OnnxRuntimeBackend() = default;
OnnxRuntimeBackend::OnnxRuntimeBackend(OnnxRuntimeBackend&&) noexcept = default;
OnnxRuntimeBackend& OnnxRuntimeBackend::operator=(OnnxRuntimeBackend&&) noexcept = default;

backend::BackendInfo OnnxRuntimeBackend::info() const {
    backend::BackendInfo result;
    result.name = "onnxruntime";
    result.version = Ort::GetVersionString();
    result.available = true;
    result.devices = {{"CPU", "CPU"}};
    result.precisions = {model::Precision::Fp32};
    result.modelFormats = {"onnx"};
    result.supportsDynamicShapes = true;
    return result;
}

Status OnnxRuntimeBackend::load(const model::ModelArtifact& artifact,
                                const backend::BackendConfig& config) {
    unload();
    if (config.device != "CPU" || config.precision != model::Precision::Fp32) {
        return Status::error(ErrorCode::BackendUnavailable,
                             "ONNX Runtime 0.1 backend supports only CPU/FP32");
    }
    const auto modelFile = artifact.files.find("model");
    if (modelFile == artifact.files.end()) {
        return Status::error(ErrorCode::InvalidModelConfig,
                             "ONNX Runtime artifact requires files.model");
    }
    std::error_code error;
    if (!std::filesystem::is_regular_file(modelFile->second, error)) {
        return Status::error(ErrorCode::FileNotFound,
                             "ONNX model file not found: " + modelFile->second.string());
    }
    try {
        Ort::SessionOptions options;
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        if (config.intraOpThreads > 0) {
            options.SetIntraOpNumThreads(config.intraOpThreads);
        }
        impl_->session = std::make_unique<Ort::Session>(
            impl_->environment, modelFile->second.c_str(), options);

        Ort::AllocatorWithDefaultOptions allocator;
        impl_->inputNames.clear();
        impl_->outputNames.clear();
        impl_->inputShapes.clear();
        for (std::size_t index = 0; index < impl_->session->GetInputCount(); ++index) {
            const auto name = impl_->session->GetInputNameAllocated(index, allocator);
            impl_->inputNames.emplace_back(name.get());
            const auto type = impl_->session->GetInputTypeInfo(index).GetTensorTypeAndShapeInfo();
            if (type.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                unload();
                return Status::error(ErrorCode::ModelLoadFailure,
                                     "ONNX input tensor must have Float32 element type");
            }
            impl_->inputShapes.push_back(type.GetShape());
        }
        for (std::size_t index = 0; index < impl_->session->GetOutputCount(); ++index) {
            const auto name = impl_->session->GetOutputNameAllocated(index, allocator);
            impl_->outputNames.emplace_back(name.get());
            const auto type = impl_->session->GetOutputTypeInfo(index).GetTensorTypeAndShapeInfo();
            if (type.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                unload();
                return Status::error(ErrorCode::ModelLoadFailure,
                                     "ONNX output tensor must have Float32 element type");
            }
        }
        auto status = validateNames(artifact.inputNames, impl_->inputNames, "input");
        if (!status.ok()) {
            unload();
            return status;
        }
        status = validateNames(artifact.outputNames, impl_->outputNames, "output");
        if (!status.ok()) {
            unload();
            return status;
        }
        if (!artifact.inputNames.empty()) impl_->inputNames = artifact.inputNames;
        if (!artifact.outputNames.empty()) impl_->outputNames = artifact.outputNames;
        return Status::success();
    } catch (const Ort::Exception& exception) {
        unload();
        return Status::error(ErrorCode::ModelLoadFailure,
                             "ONNX Runtime model load failed: " +
                                 std::string(exception.what()));
    }
}

Result<std::vector<Tensor>> OnnxRuntimeBackend::infer(const std::vector<Tensor>& inputs) {
    if (!impl_->session) {
        return Status::error(ErrorCode::InferenceFailure,
                             "ONNX Runtime infer called before load");
    }
    if (inputs.size() != impl_->inputNames.size()) {
        return Status::error(ErrorCode::TensorShapeMismatch,
                             "ONNX Runtime input count does not match the model");
    }
    try {
        const auto memory = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> values;
        std::vector<const char*> inputNames;
        values.reserve(inputs.size());
        inputNames.reserve(inputs.size());
        for (std::size_t index = 0; index < inputs.size(); ++index) {
            const auto status = inputs[index].validate();
            if (!status.ok()) return status;
            if (!shapeMatches(impl_->inputShapes[index], inputs[index].shape)) {
                return Status::error(
                    ErrorCode::TensorShapeMismatch,
                    "ONNX input shape mismatch for input='" + impl_->inputNames[index] +
                        "': expected " + formatShape(impl_->inputShapes[index]) +
                        ", actual " + formatShape(inputs[index].shape));
            }
            values.push_back(Ort::Value::CreateTensor<float>(
                memory, const_cast<float*>(inputs[index].data.data()), inputs[index].data.size(),
                inputs[index].shape.data(), inputs[index].shape.size()));
            inputNames.push_back(impl_->inputNames[index].c_str());
        }
        std::vector<const char*> outputNames;
        outputNames.reserve(impl_->outputNames.size());
        for (const auto& name : impl_->outputNames) outputNames.push_back(name.c_str());

        auto ortOutputs = impl_->session->Run(Ort::RunOptions{nullptr}, inputNames.data(),
                                              values.data(), values.size(), outputNames.data(),
                                              outputNames.size());
        std::vector<Tensor> outputs;
        outputs.reserve(ortOutputs.size());
        for (std::size_t index = 0; index < ortOutputs.size(); ++index) {
            if (!ortOutputs[index].IsTensor()) {
                return Status::error(ErrorCode::TensorShapeMismatch,
                                     "ONNX Runtime output is not a tensor");
            }
            const auto info = ortOutputs[index].GetTensorTypeAndShapeInfo();
            if (info.GetElementType() != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                return Status::error(ErrorCode::TensorShapeMismatch,
                                     "ONNX Runtime output is not Float32");
            }
            Tensor output;
            output.name = impl_->outputNames[index];
            output.shape = info.GetShape();
            const auto count = info.GetElementCount();
            const float* data = ortOutputs[index].GetTensorData<float>();
            output.data.assign(data, data + count);
            outputs.push_back(std::move(output));
        }
        return outputs;
    } catch (const Ort::Exception& exception) {
        return Status::error(ErrorCode::InferenceFailure,
                             "ONNX Runtime inference failed: " +
                                 std::string(exception.what()));
    }
}

void OnnxRuntimeBackend::unload() noexcept {
    impl_->session.reset();
    impl_->inputNames.clear();
    impl_->outputNames.clear();
    impl_->inputShapes.clear();
}

}  // namespace odf::backends
