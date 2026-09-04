# Qt desktop application

`odf_desktop` is a Qt 6 Widgets client of the public ODF SDK. It contains no YOLO tensor parser
and does not include ONNX Runtime headers in widgets.

## Runtime architecture

```text
CameraWorker (QThread) -> capacity-one frame handoff -> InferenceWorker (joined std::thread)
                                                    -> DetectionPacket(frame + result + generation)
                                                    -> AppController -> MainWindow/viewport
```

Camera capture and model inference never run on the GUI thread. Replacing a pending frame
increments the dropped-frame count. A model load request increments an application generation;
results from an older generation are discarded. Each result carries its original owned frame, so
boxes cannot be painted over a newer camera image.

The controller implements independent model and source states. Closing the window stops capture,
joins the camera thread, stops and joins inference, unloads the detector, then destroys the UI.

## User workflow

1. Start `odf_desktop.exe`. The app searches `--models`, `ODF_MODEL_ROOT`,
   `<application>/models`, then the installed share directory.
2. Select Framework, Model, Backend, CPU, and FP32, then choose **Load model**. Changing a combo
   does not reload until that button is pressed.
3. Choose **Open image…**, or choose a camera index and **Start camera**.
4. Adjust confidence, IoU, and maximum detections. Use search, **Select all**, **Clear**, or
   individual class checkboxes. Clearing every class deliberately displays zero boxes without
   unloading the model.
5. Read actual preprocess, inference, postprocess, total latency, FPS, object count, and dropped
   frame metrics in the right panel.

Settings persist through `QSettings`: model/backend, camera index, thresholds, class selection,
last image directory, window geometry, and model root.

## Command line

```powershell
odf_desktop.exe --models C:\path\to\models --model yolo26n --backend onnxruntime
odf_desktop.exe --models C:\path\to\models --image C:\path\to\image.jpg
odf_desktop.exe --models C:\path\to\models --camera 0
```

`--allow-unvalidated-model` is a developer-only override. It is off by default and logs a warning.

If an artifact is missing, the GUI still opens and reports the exact missing file. If no registry
can be found, it offers a directory picker and remains open after cancellation.

