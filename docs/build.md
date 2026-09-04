# Build and test

## Requirements

- CMake 3.24 or newer
- A C++17 compiler (MSVC 2022, recent GCC, or recent Clang)
- No external dependency for the default build
- ONNX Runtime C/C++ distribution only when `ODF_ENABLE_ONNXRUNTIME=ON`
- Qt 6 Widgets and OpenCV 4 only when building the desktop/image/camera applications

The build never downloads SDKs or model files.

## Windows

From a normal PowerShell prompt:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DODF_BUILD_TESTS=ON -DODF_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

From an x64 Native Tools prompt, a single-config alternative is:

```bat
cmake -S . -B build-nmake -G "NMake Makefiles" -DODF_BUILD_TESTS=ON
cmake --build build-nmake
ctest --test-dir build-nmake --output-on-failure
```

## Linux

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DODF_BUILD_TESTS=ON -DODF_BUILD_EXAMPLES=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## ONNX Runtime CPU backend

Download or build a version-matched official ONNX Runtime C/C++ distribution, then configure:

```powershell
cmake -S . -B build-ort -DODF_ENABLE_ONNXRUNTIME=ON `
  -DONNXRUNTIME_ROOT=C:\sdk\onnxruntime
cmake --build build-ort --config Release
```

`ONNXRUNTIME_ROOT` must contain the C++ headers and `onnxruntime` library. Configuration fails
with an actionable message if either is absent. The 0.1 backend supports CPU and FP32 only.

The ncnn, OpenVINO, TensorRT, and Paddle backend flags currently fail deliberately when enabled;
this is safer than producing a target that cannot perform real inference.

## Qt desktop, image CLI, and camera CLI

```powershell
cmake -S . -B build-desktop -G "Visual Studio 17 2022" -A x64 `
  -DODF_BUILD_DESKTOP=ON `
  -DODF_BUILD_TESTS=ON `
  -DODF_BUILD_EXAMPLES=ON `
  -DODF_ENABLE_ONNXRUNTIME=ON `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.11.2\msvc2022_64 `
  -DOpenCV_DIR=C:\SDK\opencv-4.14.0\opencv\build\x64\vc16\lib `
  -DONNXRUNTIME_ROOT=C:\SDK\onnxruntime-1.26.0
cmake --build build-desktop --config Release --parallel
ctest --test-dir build-desktop -C Release --output-on-failure
```

Use your installed paths; no machine-specific path is compiled into the source. The desktop
build runs `windeployqt` when it is present and stages OpenCV, ONNX Runtime, Qt plugins, and the
model registry beside the executable. `scripts/deploy_windows.ps1` creates a separate
distribution directory.

Optional real-model CTest is opt-in and never downloads an artifact:

```powershell
cmake -S . -B build-desktop `
  -DODF_ENABLE_REAL_MODEL_TESTS=ON `
  -DODF_TEST_MODEL_ROOT="$PWD/models" `
  -DODF_TEST_IMAGE="$PWD/validation-output/bus.jpg"
```

The default headless build remains independent of Qt, OpenCV, and ONNX Runtime.

## Installation

```powershell
cmake --install build --config Release --prefix C:\odf
```

The install includes public headers, `odf::core`, `odf::torch_core`, `odf::paddle_core`, CMake
package files, registry metadata/labels, and `odf_desktop` when enabled. The ONNX backend target is exported when it was built.
`tests/package_consumer` is a minimal downstream `find_package(odf CONFIG)` smoke project.

## Verified command in this repository

The development environment contained duplicate case variants of the Windows `Path` variable,
which breaks MSBuild's environment dictionary. Verification therefore used the NMake generator
with an absolute `CMAKE_MAKE_PROGRAM`. This is an environment workaround, not a project
requirement. The final result was a clean MSVC 19.44 build and a passing CTest run.
