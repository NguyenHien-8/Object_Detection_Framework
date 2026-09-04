#include "odf/models/register_adapters.hpp"

#include "odf/models/picodet_adapter.hpp"

#include <memory>

namespace odf::models {

Status registerPaddleAdapters(pipeline::DetectorFactory& factory) {
    return factory.registerAdapter(model::ModelFamily::PicoDet, [] {
        return std::make_unique<PicoDetAdapter>();
    });
}

}  // namespace odf::models

