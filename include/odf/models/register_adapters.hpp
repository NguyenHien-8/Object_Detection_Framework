#pragma once

#include "odf/pipeline/detector_factory.hpp"

namespace odf::models {

Status registerTorchAdapters(pipeline::DetectorFactory& factory);
Status registerPaddleAdapters(pipeline::DetectorFactory& factory);

}  // namespace odf::models

