#pragma once

#include "odf/image/image.hpp"
#include "odf/status.hpp"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>

namespace odf::camera {

struct FrameBufferStatistics {
    std::uint64_t captured{0};
    std::uint64_t consumed{0};
    std::uint64_t dropped{0};
};

/** Capacity-one frame handoff that overwrites stale camera frames. */
class LatestFrameBuffer {
public:
    Status push(image::ImageFrame frame);
    Result<image::ImageFrame> waitAndTake();
    void close() noexcept;
    [[nodiscard]] FrameBufferStatistics statistics() const noexcept;

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::optional<image::ImageFrame> latest_;
    FrameBufferStatistics statistics_;
    bool closed_{false};
};

}  // namespace odf::camera

