# Backends

| Backend | Build target | 0.1 state | Devices / precision |
|---|---|---|---|
| ONNX Runtime | `odf_backend_onnxruntime` | Experimental, implemented but not locally integration-tested | CPU / FP32 |
| ncnn | — | Planned; configure-time rejection | — |
| OpenVINO | — | Planned; configure-time rejection | — |
| TensorRT | — | Planned; configure-time rejection | — |
| Paddle Inference | — | Planned; configure-time rejection | — |

The backend interface reports availability, version, devices, precisions, formats, dynamic-shape
support, CUDA, and Vulkan capabilities. `DetectorFactory` also checks the model/backend pairing.
There is no automatic fallback.

The ONNX Runtime backend reads real session tensor names and element types, validates them against
metadata, accepts named contiguous FP32 tensors, and copies validated FP32 outputs into the
runtime-neutral `Tensor`. It rejects missing files, wrong device/precision, count mismatches,
non-tensor outputs, and runtime exceptions with categorized errors.

The other backend options are present so build intent is explicit, but they stop configuration
until their native implementation exists. This avoids an “available” backend that only returns
dummy boxes or silently uses another runtime.

