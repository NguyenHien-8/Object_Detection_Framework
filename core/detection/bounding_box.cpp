#include "odf/detection/bounding_box.hpp"

#include <algorithm>
#include <cmath>

namespace odf::detection {

float BoundingBox::width() const noexcept { return std::max(0.0F, x2 - x1); }
float BoundingBox::height() const noexcept { return std::max(0.0F, y2 - y1); }
float BoundingBox::area() const noexcept { return width() * height(); }

bool BoundingBox::valid() const noexcept {
    return std::isfinite(x1) && std::isfinite(y1) && std::isfinite(x2) &&
           std::isfinite(y2) && x2 > x1 && y2 > y1;
}

BoundingBox BoundingBox::clamped(float imageWidth, float imageHeight) const noexcept {
    if (!(imageWidth > 0.0F) || !(imageHeight > 0.0F)) {
        return {};
    }
    return {std::clamp(x1, 0.0F, imageWidth),
            std::clamp(y1, 0.0F, imageHeight),
            std::clamp(x2, 0.0F, imageWidth),
            std::clamp(y2, 0.0F, imageHeight)};
}

float intersectionOverUnion(const BoundingBox& lhs, const BoundingBox& rhs) noexcept {
    if (!lhs.valid() || !rhs.valid()) {
        return 0.0F;
    }
    const BoundingBox intersection{std::max(lhs.x1, rhs.x1),
                                   std::max(lhs.y1, rhs.y1),
                                   std::min(lhs.x2, rhs.x2),
                                   std::min(lhs.y2, rhs.y2)};
    const float intersectionArea = intersection.area();
    const float unionArea = lhs.area() + rhs.area() - intersectionArea;
    return unionArea > 0.0F ? intersectionArea / unionArea : 0.0F;
}

}  // namespace odf::detection

