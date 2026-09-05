# Backends

| Backend | CMake option | Build target | Current state | Device/precision |
|---|---|---|---|---|
| ONNX Runtime | `ODF_ENABLE_ONNXRUNTIME` | `odf_backend_onnxruntime` | Implemented MVP | CPU/FP32 |
| ncnn | `ODF_ENABLE_NCNN` | — | Planned; configuration rejects ON | — |
| OpenVINO | `ODF_ENABLE_OPENVINO` | — | Planned; configuration rejects ON | — |
| TensorRT | `ODF_ENABLE_TENSORRT` | — | Planned; configuration rejects ON | — |
| Paddle Inference | `ODF_ENABLE_PADDLE` | — | Planned; configuration rejects ON | — |

`IInferenceBackend` reports actual availability, version, devices, precisions, formats,
dynamic-shape support, CUDA, and Vulkan capabilities. `DetectorFactory` verifies the requested
model/backend pairing. An unavailable backend is neither displayed as usable nor replaced by a
different runtime.

The ONNX Runtime backend validates the SDK at configure time. At load time it creates a real
session, reads tensor names and element types, compares them with the profile, and accepts the
declared CPU/FP32 configuration. At inference it validates named contiguous FP32 input tensors,
runs the graph, and copies validated FP32 outputs into ODF's runtime-neutral `Tensor`. Missing
files, unsupported device/precision, name/type/count mismatches, non-tensor outputs, and runtime
exceptions are returned as categorized errors.

The validated tuple is YOLO26n + ONNX Runtime 1.26.0 + CPU/FP32 with input `images`
`[1,3,640,640]` and output `output0` `[1,300,6]`.
