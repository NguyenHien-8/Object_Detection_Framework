# Build and test

## Requirements

- CMake 3.24 or newer
- Visual Studio 2022 with the x64 C++ toolset
- Qt 6.5 or newer, MSVC2022 x64, with Widgets
- OpenCV 4.x x64 with `core`, `imgproc`, `imgcodecs`, `videoio`, and `highgui`
- Official ONNX Runtime C/C++ CPU x64 SDK
- A local model artifact for real inference; the build does not download one

Keep all Desktop outputs under `build-desktop/`. It is ignored by Git and can be removed without
changing source or local model artifacts.

## Configure

Run from the repository root in PowerShell. A backtick must be the final character on every
continued line.

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
```

Use the installed paths on the current machine. `ONNXRUNTIME_ROOT` must contain the C++ headers,
import library, and runtime DLL; a Python wheel is not a replacement.

## Build Release

```powershell
cmake --build build-desktop --config Release --parallel
```

The Visual Studio generator is multi-configuration, so `--config Release` is required for both
build and test commands. Important outputs are:

| Program | Path below repository root |
|---|---|
| Desktop | `build-desktop\desktop\Qt\Release\odf_desktop.exe` |
| Image CLI | `build-desktop\examples\image_detection\Release\odf_image_detection.exe` |
| Camera CLI | `build-desktop\examples\camera_detection\Release\odf_camera_detection.exe` |
| Registry inspector | `build-desktop\examples\Release\odf_registry_inspect.exe` |
| Tests | `build-desktop\tests\Release\odf_*_tests.exe` |

Find the Desktop executable without assuming its configuration path:

```powershell
Get-ChildItem -LiteralPath .\build-desktop -Recurse -Filter odf_desktop.exe |
  Select-Object FullName, LastWriteTime
```

The Desktop post-build steps run `windeployqt` when found, copy ONNX Runtime and the Release
OpenCV DLL, and copy `models/` beside the executable. The declared YOLO26n artifact should
therefore exist at:

```text
build-desktop/desktop/Qt/Release/models/yolo26/yolo26n/model.onnx
```

## Test

```powershell
ctest --test-dir build-desktop -C Release --output-on-failure
```

The standard Desktop configuration defines five tests:

1. `odf_unit_tests`: registry, preprocessing, geometry, NMS, adapters, benchmark primitives;
2. `odf_app_tests`: CLI/model-root/runtime catalog and OpenCV integration;
3. `odf_desktop_tests`: Qt bridge and class-filter behavior offscreen;
4. `odf_desktop_smoke`: headless launch with the normal registry;
5. `odf_desktop_missing_model_smoke`: headless missing-artifact error behavior.

These tests do not require a webcam. They also do not execute the real YOLO26n graph unless the
configuration was created with `ODF_ENABLE_REAL_MODEL_TESTS=ON`, a valid
`ODF_TEST_MODEL_ROOT`, and a real `ODF_TEST_IMAGE`. That opt-in adds `odf_real_yolo26n`.

## Deliberately unsupported switches

`ODF_ENABLE_NCNN`, `ODF_ENABLE_OPENVINO`, `ODF_ENABLE_TENSORRT`, and `ODF_ENABLE_PADDLE`
currently stop configuration with an explanatory error. They do not create placeholder runtime
targets. `ODF_ENABLE_CUDA` and `ODF_ENABLE_VULKAN` do not make the current ONNX Runtime backend
GPU-capable; the validated backend is CPU/FP32 only.

## Clean rebuild

Only when a clean reconfiguration is intended, delete the exact build directory and rerun the
configure/build/test commands:

```powershell
Remove-Item -LiteralPath .\build-desktop -Recurse -Force
```

Do not delete `models/yolo26/yolo26n/model.onnx` unless it can be regenerated or restored. Model
artifacts are ignored by Git.

## Install SDK artifacts

```powershell
cmake --install build-desktop --config Release --prefix C:\odf
```

Installation exports the built ODF libraries, public headers, CMake package files, metadata and
labels, plus `odf_desktop` when enabled. Use the deployment script—not `cmake --install` alone—
to assemble a redistributable Windows Desktop folder with native runtime DLLs.
