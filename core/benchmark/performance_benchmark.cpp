#include "odf/benchmark/performance_benchmark.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>

namespace odf::benchmark {
namespace {

Status checkCancelled(const std::atomic_bool* cancelled) {
    return cancelled != nullptr && cancelled->load(std::memory_order_relaxed)
               ? Status::error(ErrorCode::Cancelled, "performance benchmark cancelled")
               : Status::success();
}

void writeOptional(std::ostream& stream, const std::optional<std::uint64_t>& value) {
    if (value) stream << *value;
    else stream << "null";
}

void writeStatistics(std::ostream& stream, const Statistics& value) {
    stream << "{\"mean\":" << value.mean << ",\"min\":" << value.minimum
           << ",\"max\":" << value.maximum << ",\"p50\":" << value.p50
           << ",\"p95\":" << value.p95 << ",\"p99\":" << value.p99 << '}';
}

std::string escaped(std::string value) {
    std::string result;
    result.reserve(value.size());
    for (const char character : value) {
        if (character == '"' || character == '\\') result.push_back('\\');
        result.push_back(character);
    }
    return result;
}

}  // namespace

Result<BenchmarkResult> runPerformanceBenchmark(
    pipeline::IDetector& detector,
    const image::ImageFrame& sourceFrame,
    const PerformanceBenchmarkConfig& config,
    const std::atomic_bool* cancelled) {
    if (!detector.isLoaded()) {
        return Status::error(ErrorCode::InvalidArgument,
                             "performance benchmark requires a loaded detector");
    }
    if (config.measuredIterations == 0U || config.measuredIterations > 1000000U ||
        config.warmupCount > 1000000U) {
        return Status::error(ErrorCode::InvalidArgument,
                             "benchmark counts must be non-zero/reasonable");
    }
    auto frame = sourceFrame;
    for (std::size_t iteration = 0; iteration < config.warmupCount; ++iteration) {
        const auto cancellation = checkCancelled(cancelled);
        if (!cancellation.ok()) return cancellation;
        ++frame.frameId;
        auto result = detector.detect(frame, config.inferenceOptions);
        if (!result.ok()) {
            return Status::error(result.status().code(),
                                 "benchmark warmup failed: " + result.status().message());
        }
    }

    std::vector<double> preprocess;
    std::vector<double> inference;
    std::vector<double> postprocess;
    std::vector<double> total;
    preprocess.reserve(config.measuredIterations);
    inference.reserve(config.measuredIterations);
    postprocess.reserve(config.measuredIterations);
    total.reserve(config.measuredIterations);
    for (std::size_t iteration = 0; iteration < config.measuredIterations; ++iteration) {
        const auto cancellation = checkCancelled(cancelled);
        if (!cancellation.ok()) return cancellation;
        ++frame.frameId;
        auto result = detector.detect(frame, config.inferenceOptions);
        if (!result.ok()) {
            return Status::error(result.status().code(),
                                 "benchmark measured iteration failed: " +
                                     result.status().message());
        }
        preprocess.push_back(result.value().timings.preprocessMs);
        inference.push_back(result.value().timings.inferenceMs);
        postprocess.push_back(result.value().timings.postprocessMs);
        total.push_back(result.value().timings.totalMs);
    }

    auto preprocessStats = calculateStatistics(preprocess);
    auto inferenceStats = calculateStatistics(inference);
    auto postprocessStats = calculateStatistics(postprocess);
    auto totalStats = calculateStatistics(total);
    if (!preprocessStats.ok()) return preprocessStats.status();
    if (!inferenceStats.ok()) return inferenceStats.status();
    if (!postprocessStats.ok()) return postprocessStats.status();
    if (!totalStats.ok()) return totalStats.status();

    BenchmarkResult result;
    const auto model = detector.modelInfo();
    result.modelId = model ? model->id : std::string{};
    result.backend = config.backend;
    result.device = config.device;
    result.precision = config.precision;
    result.inputWidth = model ? model->input.width : 0;
    result.inputHeight = model ? model->input.height : 0;
    result.warmupCount = config.warmupCount;
    result.measuredIterations = config.measuredIterations;
    result.preprocessMs = preprocessStats.value();
    result.inferenceMs = inferenceStats.value();
    result.postprocessMs = postprocessStats.value();
    result.totalMs = totalStats.value();
    result.framesPerSecond = result.totalMs.mean > 0.0 ? 1000.0 / result.totalMs.mean : 0.0;
    result.modelArtifactBytes = config.modelArtifactBytes;
    result.timestampUnixMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();
    return result;
}

Status writeBenchmarkJson(const BenchmarkResult& result,
                          const std::filesystem::path& destination) {
    std::ofstream stream(destination, std::ios::binary | std::ios::trunc);
    if (!stream) {
        return Status::error(ErrorCode::InvalidArgument,
                             "cannot open benchmark destination: " + destination.string());
    }
    stream << std::setprecision(10) << "{\n"
           << "  \"schema_version\": 1,\n"
           << "  \"model_id\": \"" << escaped(result.modelId) << "\",\n"
           << "  \"backend\": \"" << escaped(result.backend) << "\",\n"
           << "  \"device\": \"" << escaped(result.device) << "\",\n"
           << "  \"precision\": \"" << escaped(result.precision) << "\",\n"
           << "  \"input\": [" << result.inputWidth << ',' << result.inputHeight << "],\n"
           << "  \"warmup_count\": " << result.warmupCount << ",\n"
           << "  \"measured_iterations\": " << result.measuredIterations << ",\n"
           << "  \"model_load_ms\": " << result.modelLoadMs << ",\n"
           << "  \"preprocess_ms\": ";
    writeStatistics(stream, result.preprocessMs);
    stream << ",\n  \"inference_ms\": ";
    writeStatistics(stream, result.inferenceMs);
    stream << ",\n  \"postprocess_ms\": ";
    writeStatistics(stream, result.postprocessMs);
    stream << ",\n  \"total_ms\": ";
    writeStatistics(stream, result.totalMs);
    stream << ",\n  \"fps\": " << result.framesPerSecond
           << ",\n  \"process_ram_bytes\": ";
    writeOptional(stream, result.processRamBytes);
    stream << ",\n  \"vram_bytes\": ";
    writeOptional(stream, result.vramBytes);
    stream << ",\n  \"model_artifact_bytes\": ";
    writeOptional(stream, result.modelArtifactBytes);
    stream << ",\n  \"timestamp_unix_ms\": " << result.timestampUnixMs << "\n}\n";
    if (!stream) {
        return Status::error(ErrorCode::Internal,
                             "failed while writing benchmark JSON: " + destination.string());
    }
    return Status::success();
}

}  // namespace odf::benchmark

