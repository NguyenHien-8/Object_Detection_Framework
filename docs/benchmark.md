# Benchmarking

`runPerformanceBenchmark` accepts a loaded detector and one image frame. Warmup iterations run
first and are excluded from measurements. Measured iterations collect preprocessing, inference,
postprocessing, and total latency separately from the detector's monotonic-clock timings.

The result contains mean, min, max, p50, p95, p99, FPS, model/backend/device/precision, input
resolution, iteration counts, optional artifact size, optional RAM/VRAM, and a timestamp.
`writeBenchmarkJson` persists the result with unavailable telemetry as JSON `null`, never zero.

Current limitations:

- Resource collectors are not implemented, so RAM/VRAM remain unavailable.
- Accuracy evaluation and COCO JSON export are planned.
- No benchmark number is checked in because no real runtime/model was available.
- Comparisons are only meaningful on the same machine, inputs, warmup policy, and precision.

Benchmark cancellation is cooperative through an atomic flag checked between iterations. UI work
must not be performed inside the measured detector call.

