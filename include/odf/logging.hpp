#pragma once

#include <functional>
#include <string>

namespace odf::logging {

enum class Level { Trace, Debug, Info, Warning, Error };
using Sink = std::function<void(Level, const std::string&)>;

/** Thread-safe process logging configuration. */
void setSink(Sink sink);
void setMinimumLevel(Level level) noexcept;
void log(Level level, const std::string& message);
const char* levelName(Level level) noexcept;

}  // namespace odf::logging

