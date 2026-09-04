# Architecture

## Dependency direction

`odf_core` owns runtime-neutral tensors, images, detections, status objects, model metadata,
preprocessing, NMS, the detector pipeline, logging, and benchmark statistics. It exposes no Qt,
CUDA, ncnn, Paddle, OpenVINO, TensorRT, or ONNX Runtime headers.

`odf_torch_core` contains `NanoDetAdapter` and `Yolo26Adapter`. `odf_paddle_core` contains
`PicoDetAdapter`. Adapters interpret model output but do not own runtime sessions. Backend
libraries execute tensors but do not decode boxes.

```text
CLI / Qt desktop client
        |
        v
DetectorFactory ---- ModelRegistry
        |
        v
DetectorPipeline
   |          |
   v          v
adapter     backend
```

Creators are registered as functions in `DetectorFactory`; there is no model/backend switch in
application code. Compatibility is checked against `ModelSpec::supportedBackends` before a
pipeline is created.

## Inference lifecycle

1. Validate metadata, deployment status, backend capability, and artifact mapping.
2. Load the backend while the detector's serialization mutex is held.
3. Validate the image and inference options.
4. Preprocess to one or more named tensors and retain an exact coordinate transform.
5. Execute the backend and validate all output buffers against their shapes.
6. Decode with the model adapter, using NMS only when metadata requires it.
7. Apply user class filtering and the maximum-detection limit.
8. Return stage timings plus frame, model-generation, and model IDs.

Each `DetectorPipeline` instance is serialized: concurrent `load`, `detect`, and `unload` calls
do not enter a vendor session simultaneously. `modelInfo()` returns a copy to avoid a dangling
pointer during model switching. Applications must reject results with `isCurrentResult()` after
switching a model or moving to a newer frame.

## Image boundary

The public core uses an owned, three-channel `odf::image::Image`. This keeps the SDK buildable on
systems without OpenCV and prevents OpenCV from becoming part of the core ABI. A future OpenCV
application adapter converts `cv::Mat` at the boundary; image/camera clients can use OpenCV 4
without introducing a dependency from `odf_core` back to the application.

## Decoder contracts

- NanoDet accepts one concatenated `[1, points, classes + 4*(reg_max+1)]` DFL output. Point count
  must exactly match the configured input and strides.
- YOLO26 end-to-end accepts `[1, N, 6]` in `xyxy, confidence, class_id` form and never runs NMS.
- YOLO26 one-to-many accepts `[1, 4+classes, predictions]`, decodes `xywh` and applies NMS.
- PicoDet currently accepts Paddle graph-postprocessed `bbox [N,6]` rows in
  `class_id, confidence, xyxy` order and optional `bbox_num` for batch size one.

Every contract rejects mismatched rank, batch, channels, point counts, NaN/Inf, invalid classes,
and impossible confidence values.
