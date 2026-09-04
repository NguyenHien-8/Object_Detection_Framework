#pragma once

#include "odf/benchmark/statistics.hpp"
#include "odf/pipeline/detector.hpp"

#include <atomic>
#include <filesystem>

namespace odf::benchmark {

struct PerformanceBenchmarkConfig {
    std::size_t warmupCount{5};
    std::size_t measuredIterations{50};
    detection::InferenceOptions inferenceOptions;
    std::string backend;
    std::string device{"CPU"};
    std::string precision{"FP32"};
    std::optional<std::uint64_t> modelArtifactBytes;
};

/** Runs warmup separately and aggregates detector stage timings with a monotonic clock. */
Result<BenchmarkResult> runPerformanceBenchmark(
    pipeline::IDetector& detector,
    const image::ImageFrame& frame,
    const PerformanceBenchmarkConfig& config,
    const std::atomic_bool* cancelled = nullptr);

/** Persists a benchmark as standalone JSON; unavailable telemetry is written as null. */
Status writeBenchmarkJson(const BenchmarkResult& result,
                          const std::filesystem::path& destination);

}  // namespace odf::benchmark

