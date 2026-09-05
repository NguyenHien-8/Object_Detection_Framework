# Windows dependencies and deployment

Use x64 Release consistently for the application and every native dependency. The verified local
combination is Visual Studio 2022/MSVC 19.44, CMake 4.4.2, Qt 6.11.2 MSVC2022 x64, OpenCV 4.14.0,
and ONNX Runtime C++ 1.26.0. CMake 3.24 remains the project minimum.

## Expected SDK landmarks

```text
C:\Qt\6.11.2\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake
C:\SDK\opencv-4.14.0\opencv\build\x64\vc16\lib\OpenCVConfig.cmake
C:\SDK\onnxruntime-1.26.0\include\onnxruntime_cxx_api.h
C:\SDK\onnxruntime-1.26.0\lib\onnxruntime.lib
C:\SDK\onnxruntime-1.26.0\lib\onnxruntime.dll
```

Some ONNX Runtime distributions put the DLL under `bin`; the project checks supported locations.
Do not mix Debug DLLs (often ending in `d.dll`), x86 libraries, MinGW Qt, or another MSVC ABI with
the x64 Release executable.

## Build-tree runtime staging

Building `odf_desktop` runs `windeployqt --no-translations` when available, copies the ONNX Runtime
DLL and Release OpenCV world DLL, and stages the complete local model tree. This makes the
`Release` directory runnable on the development machine.

## Distribution folder

Create a separate `dist/` folder with:

```powershell
.\scripts\deploy_windows.ps1 `
  -BuildDirectory .\build-desktop `
  -Configuration Release `
  -QtBin "C:\Qt\6.11.2\msvc2022_64\bin" `
  -OpenCvBin "C:\SDK\opencv-4.14.0\opencv\build\x64\vc16\bin" `
  -OnnxRuntimeRoot "C:\SDK\onnxruntime-1.26.0" `
  -ModelRoot .\models
```

Before distribution, verify at least `odf_desktop.exe`, `onnxruntime.dll`, the Release
`opencv_world*.dll`, `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`,
`platforms/qwindows.dll`, registry JSON/labels, and
`models/yolo26/yolo26n/model.onnx`. Review all third-party and model licenses.

The deployment script creates or updates the exact destination passed to it; it does not prove the
folder works on a clean machine. Test the packaged folder on a machine without development paths
in `PATH` before release.

`windeployqt` may warn that `VCINSTALLDIR` is unset when invoked from ordinary PowerShell. If the
build succeeds and the required DLL/plugin checks above pass, that warning is non-fatal. Use a
Visual Studio Developer PowerShell when deployment files are missing or when producing a release
package.
