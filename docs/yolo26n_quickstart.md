# YOLO26n desktop quickstart for Windows

This workflow builds the first validated deployment path: YOLO26n → ONNX Runtime CPU/FP32 →
OpenCV image/camera I/O → Qt 6 desktop.

## 1. Install the native toolchain

1. Install Visual Studio 2022 C++ Build Tools and the x64 MSVC toolset.
2. Install CMake and Git.
3. Install a Qt 6 MSVC2022 x64 kit.
4. Install an OpenCV 4.x x64 SDK.
5. Install the official ONNX Runtime C++ CPU x64 SDK.

## 2. Create the development-only export environment

```powershell
py -3.12 -m venv .venv-export
.\.venv-export\Scripts\python.exe -m pip install ultralytics onnx onnxruntime
```

The tested environment used Python 3.12.14, Ultralytics 8.4.138, and ONNX 1.22.0. Python is not
needed to run the final C++ executables.

## 3. Export and inspect YOLO26n

```powershell
.\.venv-export\Scripts\python.exe .\scripts\export_yolo26n_onnx.py
.\.venv-export\Scripts\python.exe .\scripts\inspect_onnx.py `
  .\models\yolo26\yolo26n\model.onnx
```

Expected graph contract:

```text
input  images  [1,3,640,640] FP32
output output0 [1,300,6]     FP32
metadata end2end=True, nms=False
```

The helper prints SHA-256. Compare it with
`models/yolo26/yolo26n/VALIDATION.md`. A different hash must be revalidated; do not simply retain
`deployment_validated=true`.

## 4. Validate against the upstream reference

```powershell
.\.venv-export\Scripts\python.exe .\scripts\reference_yolo26n.py `
  --model .\models\yolo26\yolo26n\model.onnx `
  --image C:\path\to\coco-object-image.jpg `
  --json .\validation-output\reference.json `
  --render .\validation-output\reference.jpg
```

Then build and run the C++ image client on the same image and threshold. Compare class IDs,
confidence, and `xyxy` boxes before changing the profile's validation flag.

## 5. Configure and build Release

Replace the dependency roots with your locations:

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

## 6. Run image detection from the CLI

```powershell
.\build-desktop\examples\image_detection\Release\odf_image_detection.exe `
  --models .\models `
  --model yolo26n `
  --backend onnxruntime `
  --image C:\path\to\input.jpg `
  --output C:\path\to\result.jpg
```

The process returns non-zero for a missing/corrupt image, missing artifact, incompatible tensor,
or runtime exception. It prints real stage timings and each detected class/confidence/box.

## 7. Run the desktop

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe --models .\models
```

Select YOLO26n and ONNX Runtime, click **Load model**, then **Open image…**. Class filters redraw
cached detections without reloading the model. Resizing the window recomputes only the viewport
mapping, so boxes stay aligned.

You can open an image immediately:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime --image C:\path\to\input.jpg
```

## 8. Test camera capture

```powershell
.\build-desktop\examples\camera_detection\Release\odf_camera_detection.exe `
  --models .\models --model yolo26n --backend onnxruntime --camera 0
```

Press Esc or Q to stop the CLI. In the desktop, use **Start camera**, **Stop camera**, restart it,
switch back to image mode, then close the application. Camera availability is device- and
permission-dependent; ordinary CTest never requires a webcam.

## 9. Deploy

Run `scripts/deploy_windows.ps1` as shown in [windows_dependencies.md](windows_dependencies.md),
then launch `dist/odf_desktop.exe`. Confirm `platforms/qwindows.dll`, `onnxruntime.dll`, the Release
OpenCV DLL, and `models/yolo26/yolo26n/model.onnx` are present.

## Troubleshooting

- **`Qt6Config.cmake` not found:** set `CMAKE_PREFIX_PATH` to the Qt MSVC2022 x64 kit root.
- **`OpenCVConfig.cmake` not found:** set `OpenCV_DIR` to the directory containing that file.
- **ONNX Runtime header/library/DLL not found:** point `ONNXRUNTIME_ROOT` to the official C++ SDK,
  not the Python wheel.
- **`onnxruntime.dll` or OpenCV DLL missing:** run the deployment helper or copy matching Release
  x64 DLLs beside the executable.
- **Qt platform plugin `windows` missing:** run `windeployqt`; verify `platforms/qwindows.dll`.
- **`model.onnx` missing:** export it or copy it to `models/yolo26/yolo26n/model.onnx`.
- **`deployment_validated=false`:** validate the exact artifact, or use
  `--allow-unvalidated-model` only for explicit developer testing.
- **Wrong tensor shape:** run `inspect_onnx.py`; end-to-end expects `[1,300,6]`. Do not reinterpret
  a `[1,84,8400]` one-to-many tensor as end-to-end.
- **Wrong colors:** input metadata must request RGB; the OpenCV bridge owns BGR bytes and the
  metadata-driven preprocessor performs BGR→RGB.
- **Shifted boxes:** verify centered letterbox padding, resize rounding, and box restoration
  against the upstream reference.
- **No detections:** lower confidence, choose **Select all**, and confirm the image contains COCO
  classes. With every class cleared the correct result is zero displayed boxes.
- **Camera cannot open:** try another index, close other camera applications, and check Windows
  privacy permission. The app reports this failure without crashing.
- **Wrong model root:** pass an absolute `--models` path, set `ODF_MODEL_ROOT`, or select the root
  when the GUI prompts.

