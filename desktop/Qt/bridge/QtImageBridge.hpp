#pragma once

#include "odf/image/image.hpp"
#include "odf/status.hpp"

#include <QImage>

namespace odf::desktop {

Result<QImage> toQImage(const image::Image& source);

}  // namespace odf::desktop

