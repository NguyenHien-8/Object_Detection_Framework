#include "odf/benchmark/statistics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace odf::benchmark {
namespace {
double percentile(const std::vector<double>& sorted, double probability) {
    const double index = probability * static_cast<double>(sorted.size() - 1U);
    const auto lower = static_cast<std::size_t>(std::floor(index));
    const auto upper = static_cast<std::size_t>(std::ceil(index));
    const double fraction = index - static_cast<double>(lower);
    return sorted[lower] * (1.0 - fraction) + sorted[upper] * fraction;
}
}  // namespace

Result<Statistics> calculateStatistics(const std::vector<double>& samples) {
    if (samples.empty()) {
        return Status::error(ErrorCode::InvalidArgument,
                             "benchmark statistics require at least one sample");
    }
    for (const double sample : samples) {
        if (!std::isfinite(sample) || sample < 0.0) {
            return Status::error(ErrorCode::InvalidArgument,
                                 "benchmark samples must be finite and non-negative");
        }
    }
    std::vector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    Statistics result;
    result.mean = std::accumulate(sorted.begin(), sorted.end(), 0.0) /
                  static_cast<double>(sorted.size());
    result.minimum = sorted.front();
    result.maximum = sorted.back();
    result.p50 = percentile(sorted, 0.50);
    result.p95 = percentile(sorted, 0.95);
    result.p99 = percentile(sorted, 0.99);
    return result;
}

}  // namespace odf::benchmark

