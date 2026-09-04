#include "odf/logging.hpp"

#include <iostream>
#include <mutex>
#include <utility>

namespace odf::logging {
namespace {
std::mutex gMutex;
Level gMinimumLevel = Level::Info;
Sink gSink = [](Level level, const std::string& message) {
    std::clog << "[ODF " << levelName(level) << "] " << message << '\n';
};
}  // namespace

void setSink(Sink sink) {
    std::lock_guard<std::mutex> lock(gMutex);
    gSink = std::move(sink);
}

void setMinimumLevel(Level level) noexcept {
    std::lock_guard<std::mutex> lock(gMutex);
    gMinimumLevel = level;
}

void log(Level level, const std::string& message) {
    Sink sink;
    {
        std::lock_guard<std::mutex> lock(gMutex);
        if (static_cast<int>(level) < static_cast<int>(gMinimumLevel)) {
            return;
        }
        sink = gSink;
    }
    if (sink) {
        sink(level, message);
    }
}

const char* levelName(Level level) noexcept {
    switch (level) {
        case Level::Trace: return "TRACE";
        case Level::Debug: return "DEBUG";
        case Level::Info: return "INFO";
        case Level::Warning: return "WARNING";
        case Level::Error: return "ERROR";
    }
    return "UNKNOWN";
}

}  // namespace odf::logging

