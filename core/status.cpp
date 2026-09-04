#include "odf/status.hpp"

namespace odf {

const char* errorCodeName(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Ok: return "Ok";
        case ErrorCode::InvalidArgument: return "InvalidArgument";
        case ErrorCode::FileNotFound: return "FileNotFound";
        case ErrorCode::InvalidModelConfig: return "InvalidModelConfig";
        case ErrorCode::UnsupportedBackend: return "UnsupportedBackend";
        case ErrorCode::UnsupportedModelBackendPair: return "UnsupportedModelBackendPair";
        case ErrorCode::BackendUnavailable: return "BackendUnavailable";
        case ErrorCode::ModelLoadFailure: return "ModelLoadFailure";
        case ErrorCode::InferenceFailure: return "InferenceFailure";
        case ErrorCode::TensorShapeMismatch: return "TensorShapeMismatch";
        case ErrorCode::CameraFailure: return "CameraFailure";
        case ErrorCode::DatasetFailure: return "DatasetFailure";
        case ErrorCode::Cancelled: return "Cancelled";
        case ErrorCode::Internal: return "Internal";
    }
    return "Unknown";
}

}  // namespace odf

