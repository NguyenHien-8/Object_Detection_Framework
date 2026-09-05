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
2. Select **Refresh** to recover source state and re-enumerate camera devices without normally
   reloading the model.
3. Select **Open image…**, or choose a real camera name and select **Start camera**.
4. Tune confidence, IoU, and maximum detections. For the end-to-end YOLO26n graph, IoU is graph
   controlled because metadata declares `requires_nms=false`; the control remains part of the
   shared options contract.
5. Search classes, select all, clear all, or change individual classes. An empty class selection
   intentionally displays no boxes.
6. Read preprocessing, inference, postprocessing, total latency, FPS, object count, and dropped
   frame count from the metrics panel.

Class filtering redraws cached detections; it does not run the graph again. The viewport preserves
the source aspect ratio and maps original-image coordinates only during painting, so resizing the
window does not change inference results.

## States and failure behavior

The controller maintains independent model state and an acknowledged camera state machine:

```text
Idle -> Opening -> Running -> Stopping -> Idle
          \            \               -> Error
```

The Start button is disabled during Opening; the button shows `Stopping...` and remains disabled
until release acknowledgement. Loading a model while camera capture is active is queued until
that acknowledgement. Missing artifacts, invalid images, incompatible tensors, unavailable
cameras, and runtime exceptions are reported without inventing detections.

Refresh preserves the loaded model. While running, it stops and releases the old session,
invalidates pending/in-flight source generations, enumerates devices, restores the same stable
device, and waits for the first frame of a new session. While stopped it only clears stale source
state and refreshes devices. In image mode it resubmits the retained image once.

Settings stored through `QSettings` include model/backend selection, stable camera symbolic link, thresholds,
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
unloads the detector. Camera availability is hardware-, driver-, privacy-, and device-dependent;
passing CTest does not prove a physical camera works. Use the manual hardware executable described
in [Build and test](build.md) for repeated restart validation.
