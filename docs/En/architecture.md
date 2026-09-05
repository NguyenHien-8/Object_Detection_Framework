# Architecture and algorithms

ODF keeps model semantics, inference runtimes, applications, and presentation code in separate
layers. The dependency direction points inward toward runtime-neutral C++17 interfaces.

## Source layout

```text
include/odf/                 public SDK headers
core/                        image, detection, registry, pipeline, benchmark, frame buffer
frameworks/torch/            NanoDet and YOLO26 model adapters
frameworks/paddle/           PicoDet model adapter
backends/onnxruntime/        implemented ONNX Runtime CPU/FP32 backend
backends/{ncnn,...}/         planned backend placeholders and documentation
applications/                shared CLI/Desktop composition and OpenCV bridge
desktop/Qt/                  Qt 6 Widgets UI, controller, settings, workers
examples/                    registry, image, and camera executables
models/                      model.json registry, labels, local generated artifacts
scripts/                     export, inspect, reference comparison, Windows deployment
tests/                       core, application, desktop, smoke, optional real-model tests
docs/{En,Vn}/                language-separated operational documentation
```

## Build targets and dependencies

```text
odf_desktop / image CLI / camera CLI
              |
              v
       odf_app_support --------------------> OpenCV
              |
              v
      DetectorFactory + DetectorPipeline
          |                     |
          v                     v
 IModelAdapter             IInferenceBackend
 (YOLO26/NanoDet/PicoDet)  (ONNX Runtime)
          \                     /
           +------ odf_core ---+
```

`odf_core` does not include Qt, OpenCV, or vendor runtime headers. Model adapters own tensor
decoding rules; backends only load and execute graphs. The UI consumes `IDetector` and never
implements another detector or decoder.

## Registry and detector creation

At startup, `ModelRegistry` recursively scans the selected model root for `model.json`. Each
profile is schema-validated and relative label/artifact paths are resolved against that profile's
directory. `RuntimeCatalog` registers the compiled adapters and backends. `DetectorFactory` then
checks all of these before loading:

1. the model profile exists and is structurally valid;
2. `deployment_validated` is true, unless the developer explicitly enables the override;
3. an adapter is registered for the model family;
4. the requested backend is compiled, available, and declared by the profile;
5. device and precision are supported;
6. every required artifact exists and matches the session tensor contract.

There is no implicit backend fallback and no synthetic detection path.

## Inference algorithm

`DetectorPipeline::detect` serializes one detector instance with a mutex and executes these
stages:

```text
validate frame/options/classes
        |
        v
adapter.preprocess(image, model metadata)
        |
        v
backend.infer(named FP32 tensors)
        |
        v
adapter.postprocess(runtime-neutral output tensors)
        |
        +--> optional NMS when metadata requires it
        +--> selected-class filter
        +--> max-detections limit
        v
DetectionResult + per-stage monotonic timings + model generation
```

Preprocessing owns the image bytes, converts BGR to RGB when requested, uses bilinear sampling,
and applies either direct resize or centered letterbox. For the validated YOLO26n profile the
scale is `min(640/source_width, 640/source_height)`, unused pixels are filled with `114`, values
are scaled by `1/255`, and the result is written as contiguous FP32 CHW `[1,3,640,640]`. The
stored resize transform is later inverted so boxes return to original-image coordinates.

The validated YOLO26 end-to-end output is FP32 `[1,N,6]` with each row interpreted as
`x1,y1,x2,y2,confidence,class_id`. It applies confidence/class validation and coordinate restore;
the graph already performed NMS. The separate one-to-many contract is `[1,4+C,N]`, computes the
best class per prediction, restores coordinates, then uses class-aware NMS. Rank, batch, channel,
point-count, finite-value, class-range, and confidence-range mismatches return errors.

## Desktop concurrency

```text
GUI thread
  AppController -> MainWindow / DetectionViewport
       ^                         |
       | DetectionPacket         | commands
       |                         v
InferenceWorker std::thread <- capacity-one pending frame
       ^
       |
CameraWorker QObject on QThread <- Media Foundation identity + OpenCV CAP_MSMF
```

Windows camera discovery uses `MFEnumDeviceSources`; the visible value is the Media Foundation
FriendlyName, the persistent value is its symbolic link, and only the matching internal
`CAP_MSMF` enumeration index reaches OpenCV. No numeric placeholder devices are created.

The acknowledged lifecycle is `Idle -> Opening -> Running -> Stopping -> Idle`, with failures
entering `Error`. `Running` is acknowledged only after the first valid frame. `Idle` is
acknowledged only after scheduling stops and `VideoCapture::release()` completes. Opening and
stopping watchdogs are five and three seconds respectively; they never force-kill native code.

The camera and inference work never execute on the GUI thread. Capture uses a 15 ms single-shot
timer that is rescheduled after each completed read, leaving the camera event loop able to process
queued lifecycle requests. If capture outruns inference, a new frame replaces the single pending
frame and increments `droppedFrames`. Camera session IDs and source generations reject old
frames and in-flight detections after stop, restart, source switch, or Refresh.

Refresh invalidates the source generation and re-enumerates devices without unloading the model.
For a running camera it waits for the release acknowledgement, preserves the symbolic-link ID,
and starts a new session only if that same device remains present. In image mode it resubmits the
retained in-memory image once. Changing class selection filters the cached result without another inference.
Changing confidence, IoU, or maximum detections resubmits the current still image; live camera
mode naturally applies the new options to the next frame. Shutdown stops capture, joins the Qt
camera thread, stops and joins inference, unloads the detector, and only then destroys the UI.
