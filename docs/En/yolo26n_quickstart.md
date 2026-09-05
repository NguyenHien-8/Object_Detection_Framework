# YOLO26n Windows quickstart

This is the shortest supported path from source to the validated Desktop MVP.

## 1. Confirm the model

```powershell
Test-Path -LiteralPath .\models\yolo26\yolo26n\model.onnx
Get-FileHash -Algorithm SHA256 `
  -LiteralPath .\models\yolo26\yolo26n\model.onnx
```

The expected hash is
`4f9a606c410c6c5b4bdc887f80aaebb05ae37c0d7487dace14182768508da234`. If the file is missing,
export it in a development-only Python environment:

```powershell
py -3.12 -m venv .venv-export
.\.venv-export\Scripts\python.exe -m pip install ultralytics onnx onnxruntime
.\.venv-export\Scripts\python.exe .\scripts\export_yolo26n_onnx.py
.\.venv-export\Scripts\python.exe .\scripts\inspect_onnx.py `
  .\models\yolo26\yolo26n\model.onnx
```

Expected contract: input `images` FP32 `[1,3,640,640]`; output `output0` FP32 `[1,300,6]`;
metadata `end2end=True`, `nms=False`. A new hash is unvalidated until reference comparison passes.

## 2. Configure, build, and test

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

## 3. Inspect the registry

```powershell
.\build-desktop\examples\Release\odf_registry_inspect.exe .\models
```

It should list 16 profiles and show `yolo26n` as `validated`. That flag is necessary but the hash
check above is what ties it to the current artifact.

## 4. Run image inference from CLI

```powershell
.\build-desktop\examples\image_detection\Release\odf_image_detection.exe `
  --models .\models `
  --model yolo26n `
  --backend onnxruntime `
  --image "C:\path\to\input.jpg" `
  --output "C:\path\to\result.jpg"
```

The CLI prints actual stage timings and detections and returns non-zero for a missing/corrupt
image, missing artifact, invalid tensor contract, or runtime failure.

## 5. Run Desktop

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime
```

The selected model is loaded through the worker thread. Open an image or start a camera in the UI.
The app can also open an image directly:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime `
  --image "C:\path\to\input.jpg"
```

## 6. Run camera CLI

```powershell
.\build-desktop\examples\camera_detection\Release\odf_camera_detection.exe `
  --models .\models --model yolo26n --backend onnxruntime --camera 0
```

Press Esc or Q to stop. If index 0 is unavailable, close competing camera apps, check Windows
privacy settings, and try another index. Camera success is not implied by CTest.

## 7. Troubleshooting

- `Qt6Config.cmake` missing: point `CMAKE_PREFIX_PATH` at the MSVC2022 x64 Qt kit.
- `OpenCVConfig.cmake` missing: point `OpenCV_DIR` at its containing directory.
- ONNX headers/library/DLL missing: use the official C/C++ SDK root, not the Python package.
- DLL or Qt platform plugin missing: rebuild the Desktop target or run the deployment script.
- `model.onnx` missing: export or restore it under the exact profile directory.
- Artifact rejected as unvalidated: compare the exact hash and outputs; do not permanently use the
  developer override as a deployment solution.
- Wrong shape: inspect the graph; end-to-end expects `[1,300,6]`, not `[1,84,8400]`.
- No displayed boxes: check confidence and class selection; clearing every class intentionally
  displays zero boxes.
- Shifted boxes: verify centered letterbox, resize rounding, and inverse coordinate transform.
- Wrong colors: the source is BGR and profile-driven preprocessing converts it to RGB.
