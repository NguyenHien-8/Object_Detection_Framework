#pragma once

#include "odf/status.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace odf::benchmark {

struct Statistics {
    double mean{0.0};
    double minimum{0.0};
    double maximum{0.0};
    double p50{0.0};
    double p95{0.0};
    double p99{0.0};
};

/** Computes deterministic nearest-rank latency statistics. */
Result<Statistics> calculateStatistics(const std::vector<double>& samples);

struct BenchmarkResult {
    std::string modelId;
    std::string backend;
    std::string device;
    std::string precision;
    int inputWidth{0};
    int inputHeight{0};
    std::size_t warmupCount{0};
    std::size_t measuredIterations{0};
    double modelLoadMs{0.0};
    Statistics preprocessMs;
    Statistics inferenceMs;
    Statistics postprocessMs;
    Statistics totalMs;
    double framesPerSecond{0.0};
    std::optional<std::uint64_t> processRamBytes;
    std::optional<std::uint64_t> vramBytes;
    std::optional<std::uint64_t> modelArtifactBytes;
    std::int64_t timestampUnixMs{0};
};

}  // namespace odf::benchmark
