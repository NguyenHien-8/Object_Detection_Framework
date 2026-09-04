#pragma once

#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace odf {

enum class ErrorCode {
    Ok = 0,
    InvalidArgument,
    FileNotFound,
    InvalidModelConfig,
    UnsupportedBackend,
    UnsupportedModelBackendPair,
    BackendUnavailable,
    ModelLoadFailure,
    InferenceFailure,
    TensorShapeMismatch,
    CameraFailure,
    DatasetFailure,
    Cancelled,
    Internal
};

/** Describes success or a categorized failure with diagnostic context. */
class Status {
public:
    Status() = default;
    Status(ErrorCode code, std::string message)
        : code_(code), message_(std::move(message)) {
        if (code_ == ErrorCode::Ok && !message_.empty()) {
            throw std::invalid_argument("an OK status cannot contain an error message");
        }
    }

    static Status success() { return {}; }
    static Status error(ErrorCode code, std::string message) {
        if (code == ErrorCode::Ok) {
            throw std::invalid_argument("Status::error requires a non-OK code");
        }
        return {code, std::move(message)};
    }

    [[nodiscard]] bool ok() const noexcept { return code_ == ErrorCode::Ok; }
    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }

private:
    ErrorCode code_{ErrorCode::Ok};
    std::string message_;
};

/** Holds either a value or a non-OK Status. */
template <typename T>
class Result {
public:
    Result(T value) : storage_(std::move(value)) {}
    Result(Status status) : storage_(std::move(status)) {
        if (std::get<Status>(storage_).ok()) {
            throw std::invalid_argument("Result error must not be OK");
        }
    }

    [[nodiscard]] bool ok() const noexcept { return std::holds_alternative<T>(storage_); }
    [[nodiscard]] const Status& status() const {
        if (ok()) {
            static const Status successStatus;
            return successStatus;
        }
        return std::get<Status>(storage_);
    }
    [[nodiscard]] const T& value() const { return std::get<T>(storage_); }
    [[nodiscard]] T& value() { return std::get<T>(storage_); }
    [[nodiscard]] T takeValue() { return std::move(std::get<T>(storage_)); }

private:
    std::variant<T, Status> storage_;
};

const char* errorCodeName(ErrorCode code) noexcept;

}  // namespace odf

