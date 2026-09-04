#pragma once

namespace odf::detection {

/** Axis-aligned coordinates in the original input-image coordinate space. */
struct BoundingBox {
    float x1{0.0F};
    float y1{0.0F};
    float x2{0.0F};
    float y2{0.0F};

    [[nodiscard]] float width() const noexcept;
    [[nodiscard]] float height() const noexcept;
    [[nodiscard]] float area() const noexcept;
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] BoundingBox clamped(float imageWidth, float imageHeight) const noexcept;
};

/** Computes intersection-over-union; invalid/degenerate boxes yield zero. */
[[nodiscard]] float intersectionOverUnion(const BoundingBox& lhs,
                                          const BoundingBox& rhs) noexcept;

}  // namespace odf::detection

