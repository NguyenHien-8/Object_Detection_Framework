# Qt desktop application

`odf_desktop` is a Qt 6 Widgets client over the public ODF pipeline. It contains no model decoder
and does not expose ONNX Runtime types to widgets.

## Start the application

From the repository root:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime
```

Open an image immediately:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime `
  --image "C:\path\to\image.jpg"
```

Start a camera after the model loads:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime --camera 0
```

## Model-root lookup

The common application resolver tries an explicit `--models` path, `ODF_MODEL_ROOT`,
`<executable-directory>/models`, and the installed shared-data model directory. The Desktop also
uses its saved model root and offers a directory picker when no usable registry is found. Passing
an absolute `--models` path is the most deterministic choice when diagnosing startup.

## User workflow

1. Confirm Framework, Model, Backend, CPU, and FP32, then select **Load model**. Changing a combo
   does not switch the loaded detector until this action is used.
2. Select **Open image…**, or choose a camera index and select **Start camera**.
3. Tune confidence, IoU, and maximum detections. For the end-to-end YOLO26n graph, IoU is graph
   controlled because metadata declares `requires_nms=false`; the control remains part of the
   shared options contract.
4. Search classes, select all, clear all, or change individual classes. An empty class selection
   intentionally displays no boxes.
5. Read preprocessing, inference, postprocessing, total latency, FPS, object count, and dropped
   frame count from the metrics panel.

Class filtering redraws cached detections; it does not run the graph again. The viewport preserves
the source aspect ratio and maps original-image coordinates only during painting, so resizing the
window does not change inference results.

## States and failure behavior

The controller maintains independent model states (`Unloaded`, `Loading`, `Ready`, `Error`,
`Unloading`) and source states (idle/image/camera starting/running/stopped/error). Loading a new
model first stops the camera. Missing artifacts, invalid images, incompatible tensors, unavailable
cameras, and runtime exceptions are reported in the status area without inventing detections.

Settings stored through `QSettings` include model/backend selection, camera index, thresholds,
class selection, last image directory, model root, and window geometry.

## Command-line options

```text
--models PATH
--model ID
--backend ID
--image PATH | --camera N
--output PATH
--confidence N
--iou N
--max-detections N
--log-level LEVEL
--allow-unvalidated-model
```

`--allow-unvalidated-model` is only for explicit development tests. It bypasses the registry
guard, not tensor validation, missing-file checks, or backend capability checks.

## Shutdown and camera notes

Closing the window stops camera capture, joins the camera thread, stops and joins inference, and
unloads the detector. Camera availability is hardware-, driver-, privacy-, and index-dependent;
passing CTest does not prove a physical camera works.
