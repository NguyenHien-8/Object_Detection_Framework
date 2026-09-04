# Windows dependencies

Use one architecture and build type for every native dependency. The validated setup is x64
Release with MSVC 2022.

## Required software

- Visual Studio 2022 Build Tools with Desktop development with C++ and an x64 MSVC toolset
- CMake 3.24 or newer and Git
- Qt 6 MSVC2022 x64 with Widgets (`Qt6Config.cmake` under the Qt kit)
- OpenCV 4.x x64 (`OpenCVConfig.cmake` under `build/x64/vc16/lib` for the tested SDK)
- Official ONNX Runtime C/C++ CPU x64 distribution containing
  `include/onnxruntime_cxx_api.h`, `lib/onnxruntime.lib`, and `lib/onnxruntime.dll`
- Python only for model export/inspection; it is not a runtime dependency

Validated versions: MSVC 19.44, CMake 4.4.2, Qt 6.11.2, OpenCV 4.14.0, ONNX Runtime C++
1.26.0, Python 3.12.14, Ultralytics 8.4.138, ONNX 1.22.0.

## Typical configuration

```powershell
cmake -S . -B build-desktop -G "Visual Studio 17 2022" -A x64 `
  -DODF_BUILD_DESKTOP=ON -DODF_ENABLE_ONNXRUNTIME=ON `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.11.2\msvc2022_64 `
  -DOpenCV_DIR=C:\SDK\opencv-4.14.0\opencv\build\x64\vc16\lib `
  -DONNXRUNTIME_ROOT=C:\SDK\onnxruntime-1.26.0
```

The error messages name the missing CMake package, header, import library, or DLL. Do not point a
Release executable at DLLs ending in `d.dll`.

## Distribution staging

```powershell
.\scripts\deploy_windows.ps1 `
  -BuildDirectory .\build-desktop `
  -Configuration Release `
  -QtBin C:\Qt\6.11.2\msvc2022_64\bin `
  -OpenCvBin C:\SDK\opencv-4.14.0\opencv\build\x64\vc16\bin `
  -OnnxRuntimeRoot C:\SDK\onnxruntime-1.26.0 `
  -ModelRoot .\models
```

This copies the desktop executable, Release OpenCV DLL, `onnxruntime.dll`, Qt libraries/plugins
through `windeployqt`, and model registry/artifacts.
