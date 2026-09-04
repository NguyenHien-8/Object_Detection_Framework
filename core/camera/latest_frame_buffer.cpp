#include "odf/camera/latest_frame_buffer.hpp"

#include <utility>

namespace odf::camera {

Status LatestFrameBuffer::push(image::ImageFrame frame) {
    const auto status = frame.image.validate();
    if (!status.ok()) return status;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return Status::error(ErrorCode::Cancelled,
                                 "cannot push to a closed latest-frame buffer");
        }
        ++statistics_.captured;
        if (latest_) ++statistics_.dropped;
        latest_ = std::move(frame);
    }
    condition_.notify_one();
    return Status::success();
}

Result<image::ImageFrame> LatestFrameBuffer::waitAndTake() {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [&] { return latest_.has_value() || closed_; });
    if (!latest_) {
        return Status::error(ErrorCode::Cancelled, "latest-frame buffer was closed");
    }
    auto frame = std::move(*latest_);
    latest_.reset();
    ++statistics_.consumed;
    return frame;
}

void LatestFrameBuffer::close() noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        if (latest_) ++statistics_.dropped;
        latest_.reset();
    }
    condition_.notify_all();
}

FrameBufferStatistics LatestFrameBuffer::statistics() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return statistics_;
}

}  // namespace odf::camera
