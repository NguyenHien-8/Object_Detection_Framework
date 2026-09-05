# Benchmarking

`runPerformanceBenchmark` accepts a loaded detector and one image. Warmup iterations run first
and are excluded. Measured iterations collect preprocessing, inference, postprocessing, and total
latency from the pipeline's monotonic-clock timings.

The result includes mean, minimum, maximum, p50, p95, p99, FPS, model/backend/device/precision,
input resolution, iteration counts, optional artifact size, optional RAM/VRAM, and a timestamp.
`writeBenchmarkJson` writes unavailable telemetry as JSON `null`, never as a fabricated zero.

Current limits:

- RAM and VRAM collectors are not implemented;
- accuracy evaluation and COCO JSON export are not implemented;
- generated results belong under ignored `benchmarks/results/`;
- comparisons are meaningful only with the same hardware, power mode, runtime, artifact, input,
  warmup, thresholds, and precision;
- cancellation is cooperative between iterations.

Do not compare a single Desktop FPS display with a controlled benchmark: UI rendering and camera
capture are outside the detector's measured call.
