#include "odf/app/logging_options.hpp"

#include "odf/logging.hpp"

#include <algorithm>
#include <cctype>

namespace odf::app {

Status applyLogLevel(const std::string& name) {
    std::string normalized = name;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    if (normalized == "trace") logging::setMinimumLevel(logging::Level::Trace);
    else if (normalized == "debug") logging::setMinimumLevel(logging::Level::Debug);
    else if (normalized == "info") logging::setMinimumLevel(logging::Level::Info);
    else if (normalized == "warning" || normalized == "warn") {
        logging::setMinimumLevel(logging::Level::Warning);
    } else if (normalized == "error") logging::setMinimumLevel(logging::Level::Error);
    else {
        return Status::error(ErrorCode::InvalidArgument,
                             "--log-level must be trace, debug, info, warning, or error");
    }
    return Status::success();
}

}  // namespace odf::app

