#pragma once

#include "odf/status.hpp"

#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

namespace odf {

enum class DataType { Float32, Float16, Int8, Int32, Int64 };

/** Runtime-neutral contiguous tensor. The first release stores FP32 payloads. */
struct Tensor {
    std::string name;
    std::vector<std::int64_t> shape;
    DataType dataType{DataType::Float32};
    std::vector<float> data;

    [[nodiscard]] Result<std::size_t> elementCount() const {
        if (shape.empty()) {
            return Status::error(ErrorCode::TensorShapeMismatch,
                                 "tensor shape must have at least one dimension");
        }
        std::size_t count = 1;
        for (const auto dimension : shape) {
            if (dimension <= 0) {
                return Status::error(ErrorCode::TensorShapeMismatch,
                                     "tensor dimensions must be positive");
            }
            const auto value = static_cast<std::size_t>(dimension);
            if (count > std::numeric_limits<std::size_t>::max() / value) {
                return Status::error(ErrorCode::TensorShapeMismatch,
                                     "tensor element count overflows size_t");
            }
            count *= value;
        }
        return count;
    }

    [[nodiscard]] Status validate() const {
        if (dataType != DataType::Float32) {
            return Status::error(ErrorCode::InvalidArgument,
                                 "ODF 0.1 accepts only Float32 tensor payloads");
        }
        const auto count = elementCount();
        if (!count.ok()) {
            return count.status();
        }
        if (count.value() != data.size()) {
            return Status::error(ErrorCode::TensorShapeMismatch,
                                 "tensor shape does not match its payload size");
        }
        return Status::success();
    }
};

}  // namespace odf

