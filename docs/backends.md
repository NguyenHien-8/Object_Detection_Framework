# Backends

| Backend | Build target | 0.1 state | Devices / precision |
|---|---|---|---|
| ONNX Runtime | `odf_backend_onnxruntime` | Supported MVP; validated with YOLO26n ONNX | CPU / FP32 |
| ncnn | — | Planned; configure-time rejection | — |
| OpenVINO | — | Planned; configure-time rejection | — |
| TensorRT | — | Planned; configure-time rejection | — |
| Paddle Inference | — | Planned; configure-time rejection | — |

The backend interface reports availability, version, devices, precisions, formats, dynamic-shape
support, CUDA, and Vulkan capabilities. `DetectorFactory` also checks the model/backend pairing.
There is no automatic fallback.

The ONNX Runtime 1.26.0 backend reads real session tensor names and element types, validates them against
metadata, accepts named contiguous FP32 tensors, and copies validated FP32 outputs into the
runtime-neutral `Tensor`. It rejects missing files, wrong device/precision, count mismatches,
non-tensor outputs, and runtime exceptions with categorized errors.

The validated tuple is YOLO26n + ONNX Runtime CPU + FP32 with input `images`
`[1,3,640,640]` and output `output0` `[1,300,6]`. Runtime capability registration drives the
desktop backend list; unavailable backends are not selectable.

The other backend options are present so build intent is explicit, but they stop configuration
until their native implementation exists. This avoids an “available” backend that only returns
dummy boxes or silently uses another runtime.
