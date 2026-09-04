#include "odf/models/register_adapters.hpp"

#include "odf/models/nanodet_adapter.hpp"
#include "odf/models/yolo26_adapter.hpp"

#include <memory>

namespace odf::models {

Status registerTorchAdapters(pipeline::DetectorFactory& factory) {
    auto status = factory.registerAdapter(model::ModelFamily::NanoDet, [] {
        return std::make_unique<NanoDetAdapter>();
    });
    if (!status.ok()) return status;
    return factory.registerAdapter(model::ModelFamily::Yolo26, [] {
        return std::make_unique<Yolo26Adapter>();
    });
}

}  // namespace odf::models

