# Object Detection Framework

Object Detection Framework (ODF) 0.1.0 is a C++17 SDK and Windows desktop MVP for composing
model-specific adapters with independent inference runtimes. The validated deployment path is:

```text
YOLO26n -> ONNX Runtime 1.26.0 (CPU/FP32) -> ODF pipeline
         -> OpenCV image/camera I/O -> Qt 6 Widgets desktop
```

## Current state

| Area | State |
|---|---|
| Core pipeline, registry, preprocessing, NMS, benchmark API | Implemented and unit-tested |
| YOLO26 and NanoDet adapters | Implemented; synthetic decoder tests |
| PicoDet adapter | Implemented; synthetic decoder tests |
| ONNX Runtime backend | Implemented for CPU/FP32 |
| Qt desktop, image CLI, camera CLI | Implemented MVP |
| ncnn, OpenVINO, TensorRT, Paddle Inference backends | Planned; enabling them fails configuration explicitly |
| Model registry | 16 profiles: one validated YOLO26n profile and 15 guarded templates |

The local `models/yolo26/yolo26n/model.onnx` is present and its SHA-256 matches the validation
record. Generated weights are ignored by Git, and the CMake build never downloads them. A clean
source build is reproducible once native dependencies and the model artifact have been supplied.

## Windows clean build

Use only `build-desktop/` for the desktop configuration:

```powershell
cmake -S . -B build-desktop `
  -G "Visual Studio 17 2022" -A x64 `
  -DODF_BUILD_DESKTOP=ON `
  -DODF_BUILD_TESTS=ON `
  -DODF_BUILD_EXAMPLES=ON `
  -DODF_ENABLE_ONNXRUNTIME=ON `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.11.2\msvc2022_64" `
  -DOpenCV_DIR="C:\SDK\opencv-4.14.0\opencv\build\x64\vc16\lib" `
  -DONNXRUNTIME_ROOT="C:\SDK\onnxruntime-1.26.0"

cmake --build build-desktop --config Release --parallel
ctest --test-dir build-desktop -C Release --output-on-failure
```

The executable is:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe
```

The build stages Qt, OpenCV, ONNX Runtime, registry metadata, labels, and available local model
artifacts beside the executable.

## Run

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime
```

For a complete walkthrough, architecture, algorithms, validation rules, and troubleshooting, see
the [English documentation index](docs/En/README.md).

## License boundary

ODF source is governed by the repository [`LICENSE`](LICENSE). Runtime SDKs and model artifacts
retain their own licenses; see [third-party notices](THIRD_PARTY_NOTICES_En.md).
