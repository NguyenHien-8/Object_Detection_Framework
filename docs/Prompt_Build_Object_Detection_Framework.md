# PROMPT — Build `Object_Detection_Framework`

## 0. Role

You are a senior C++ Computer Vision / Deep Learning inference engineer and software architect.

Your task is to **design and implement the complete project**:

**Repository:** `NguyenHien-8/Object_Detection_Framework`  
**Project name:** `Object_Detection_Framework`  
**Primary language:** C++  
**GUI:** Qt 6  
**Computer Vision I/O:** OpenCV  
**Build system:** CMake

The project must become a reusable **Object Detection Framework / SDK**, not merely a one-off desktop demo.

Before modifying code:

1. Inspect the entire current repository.
2. Preserve existing files and behavior unless a change is required by this specification.
3. Do not overwrite or replace the existing project license.
4. Build the project incrementally and compile/test after each major milestone.
5. Do not leave fake inference paths, hard-coded demo detections, placeholder bounding boxes, or code that silently reports success when a backend/model did not actually run.
6. If an optional third-party runtime is not installed, compile that backend only when its CMake option is enabled and provide a clear configuration error or capability status.
7. Prefer maintainable production-style code over short demonstration code.

---

# 1. Project Goal

Build a modular C++ framework that can compare and deploy multiple Object Detection models originating from two model ecosystems:

## PyTorch-origin models

### NanoDet

Support these model profiles:

- NanoDet-Plus-m-320
- NanoDet-Plus-m-416
- NanoDet-Plus-m-1.5x-320
- NanoDet-Plus-m-1.5x-416

### YOLO26

Support:

- YOLO26n
- YOLO26s
- YOLO26m
- YOLO26l
- YOLO26x

## PaddlePaddle-origin models

### PicoDet

Support:

- PicoDet-S 320
- PicoDet-S 416
- PicoDet-M 320
- PicoDet-M 416
- PicoDet-L 320
- PicoDet-L 416
- PicoDet-L 640

The framework has three main goals:

1. Run multiple object-detection model families through one common C++ API.
2. Benchmark speed/resource usage/accuracy so the user can choose the best model for a target device.
3. Expose reusable C++ libraries that can later be integrated into other projects without depending on the desktop GUI.

---

# 2. Core Architectural Principle

Do **not** create one monolithic class containing NanoDet, PicoDet, YOLO26, Qt, OpenCV camera capture, TensorRT, Paddle, and ncnn logic.

Use strict separation of concerns:

```text
                   Desktop
                      |
                  ODF Core
                      |
             Model Adapter Layer
            /         |          \
       NanoDet     PicoDet      YOLO26
            \         |          /
               Runtime API
                    |
       +------------+-------------+
       |            |             |
     ncnn        TensorRT        Paddle
       |            |             |
   ARM/CPU       NVIDIA GPU      Paddle
```

The diagram above is the primary simplified architecture.

The implementation must also allow these optional runtime backends because they are included in the requested source tree:

- ONNX Runtime
- OpenVINO
- TensorRT
- ncnn
- Paddle Inference

Therefore, the real internal architecture must be:

```text
Desktop / Example Applications
             |
         odf_core
             |
      Detector/Pipeline
             |
   +---------+----------+
   |                    |
IModelAdapter      IInferenceBackend
   |                    |
   |              +-----+-----------+-----------+----------+
   |              |                 |           |          |
 NanoDet       ncnn          ONNX Runtime   OpenVINO   TensorRT
 PicoDet                                                   |
 YOLO26                                                NVIDIA
   |
   +------------------------------------------------ Paddle
```

Model-specific code and runtime-specific code must remain independent.

---

# 3. Critical Terminology

Do not confuse **source framework** with **inference runtime**.

For this project:

- `frameworks/torch/` means model families that originate from the PyTorch ecosystem.
- `frameworks/paddle/` means model families that originate from PaddlePaddle.
- It does **not** mean that C++ inference must load `.pt` with LibTorch or execute Python.

Examples:

```text
YOLO26 (PyTorch-origin)
    -> export ONNX
    -> TensorRT
    -> C++ inference
```

```text
NanoDet (PyTorch-origin)
    -> ncnn format
    -> ncnn C++ runtime
```

```text
PicoDet (PaddlePaddle-origin)
    -> Paddle inference model
    -> Paddle Inference C++ runtime
```

The common core must never depend directly on PyTorch Python, Paddle Python, Ultralytics Python, NanoDet Python, or PaddleDetection Python.

Training/export scripts can be documented separately, but the C++ inference application must run without Python.

---

# 4. Required Project Structure

Keep this top-level structure and expand it cleanly as needed:

```text
ObjectDetectionFramework/
│
├── core/
│   ├── detection/
│   ├── pipeline/
│   ├── image/
│   ├── camera/
│   ├── benchmark/
│   ├── model_registry/
│   └── utils/
│
├── frameworks/
│   ├── torch/
│   │   ├── nanodet/
│   │   └── yolo26/
│   │
│   └── paddle/
│       └── picodet/
│
├── backends/
│   ├── onnxruntime/
│   ├── openvino/
│   ├── tensorrt/
│   ├── ncnn/
│   └── paddle/
│
├── desktop/
│   └── Qt/
│
├── models/
│
├── benchmarks/
│
├── tests/
│
├── examples/
│   ├── torch_core_example/
│   ├── paddle_core_example/
│   ├── image_detection/
│   └── camera_detection/
│
└── CMakeLists.txt
```

Add these directories if useful:

```text
include/
cmake/
docs/
scripts/
third_party/
```

However:

- Do not duplicate the same implementation under multiple directories.
- Do not vendor complete upstream frameworks unless absolutely necessary.
- Do not commit large pretrained model binaries to Git by default.
- Keep model files separate from source code.
- Add `.gitignore` rules for generated build files, downloaded models, TensorRT engine caches, temporary benchmark files, and Qt build artifacts.

---

# 5. Build Artifacts / Reusable Libraries

The design must generate reusable libraries.

At minimum:

```text
odf_core
odf_torch_core
odf_paddle_core
```

Backend libraries should be separable:

```text
odf_backend_ncnn
odf_backend_onnxruntime
odf_backend_openvino
odf_backend_tensorrt
odf_backend_paddle
```

Desktop application:

```text
odf_desktop
```

Examples must link against the same public libraries used by the desktop application.

The desktop must not contain duplicate inference implementations.

Desired dependency direction:

```text
odf_core
   ↑
odf_torch_core
odf_paddle_core

odf_core
   ↑
backend libraries

odf_core + framework core + backend
   ↑
examples / desktop
```

Never create a dependency from `odf_core` back to Qt.

---

# 6. C++ and Build Requirements

Use:

- C++17 as the minimum standard.
- CMake 3.24+ where practical.
- Qt 6 for Desktop only.
- OpenCV 4.x for image/camera operations.
- RAII for resource ownership.
- `std::unique_ptr` for exclusive ownership.
- `std::shared_ptr` only where shared ownership is genuinely required.
- `std::chrono` for timing.
- `std::thread`, `std::mutex`, `std::condition_variable`, and `std::atomic` where required.
- `std::filesystem` for paths.

Avoid:

- global mutable state;
- raw owning pointers;
- manual `new/delete` except unavoidable third-party APIs;
- busy-wait loops;
- arbitrary sleeps to synchronize threads;
- blocking inference on the Qt GUI thread;
- hidden singleton state;
- model-specific conditions scattered through generic core code.

Recommended root CMake options:

```cmake
ODF_BUILD_DESKTOP
ODF_BUILD_TESTS
ODF_BUILD_EXAMPLES

ODF_ENABLE_NCNN
ODF_ENABLE_ONNXRUNTIME
ODF_ENABLE_OPENVINO
ODF_ENABLE_TENSORRT
ODF_ENABLE_PADDLE

ODF_ENABLE_CUDA
ODF_ENABLE_VULKAN
ODF_ENABLE_BENCHMARK
```

Heavy runtimes must be optional.

A build containing only `odf_core` must still compile without TensorRT/Paddle/ncnn being installed.

---

# 7. Namespace and Naming

Use one project namespace:

```cpp
namespace odf
{
}
```

Recommended nested namespaces:

```cpp
odf::core
odf::models
odf::backends
odf::benchmark
odf::camera
```

Naming:

- Types/classes: `PascalCase`
- Functions/methods: `camelCase`
- Local variables: `camelCase`
- Constants: choose one consistent project convention
- CMake target names: lowercase `odf_*`
- Files: one consistent snake_case or PascalCase convention; do not mix styles arbitrarily.

Public APIs must have concise Doxygen comments.

---

# 8. Common Core Data Model

Create common runtime-neutral types.

## BoundingBox

Represent coordinates in original input image space.

Suggested form:

```cpp
struct BoundingBox
{
    float x1;
    float y1;
    float x2;
    float y2;
};
```

Provide helpers:

- width
- height
- area
- clamp
- valid
- IoU

Do not store backend-specific tensor coordinates in this final object.

## Detection

```cpp
struct Detection
{
    BoundingBox box;
    int classId;
    float confidence;
};
```

Optionally include:

```cpp
std::string className;
```

but avoid duplicating class-name strings for every detection if a label registry can resolve them efficiently.

## ImageFrame

Include:

- `cv::Mat`
- timestamp
- frame ID
- source metadata when useful

## InferenceOptions

At minimum:

```text
confidenceThreshold
iouThreshold
maxDetections
selectedClassIds
classAgnosticNms
```

Separate model defaults from user overrides.

## ModelInfo / ModelSpec

Must contain:

```text
id
displayName
family
sourceFramework
variant
inputWidth
inputHeight
classCount
labels
supportedBackends
defaultBackend
modelArtifact paths
preprocessing config
postprocessing config
precision support
```

## BenchmarkResult

At minimum:

```text
model ID
backend
device
precision
input resolution
warmup count
measured iterations
preprocess latency
inference latency
postprocess latency
total latency
FPS
p50 latency
p95 latency
p99 latency
RAM usage when available
VRAM usage when available
model artifact size
timestamp
```

---

# 9. Public Detector API

Expose one main generic detector contract.

A recommended design:

```cpp
class IDetector
{
public:
    virtual ~IDetector() = default;

    virtual Status load(
        const ModelSpec& model,
        const BackendConfig& backend) = 0;

    virtual DetectionResult detect(
        const cv::Mat& image,
        const InferenceOptions& options) = 0;

    virtual ModelInfo modelInfo() const = 0;

    virtual bool isLoaded() const noexcept = 0;

    virtual void unload() noexcept = 0;
};
```

Do not force this exact signature if a cleaner equivalent is justified, but preserve the abstraction.

Use explicit status/error objects or well-defined exceptions.

Do not return only `bool` for errors when useful diagnostic context would be lost.

---

# 10. Model Adapter Interface

Create an interface such as:

```cpp
class IModelAdapter
{
public:
    virtual ~IModelAdapter() = default;

    virtual ModelFamily family() const noexcept = 0;

    virtual PreprocessedInput preprocess(
        const cv::Mat& image,
        const ModelSpec& spec) = 0;

    virtual std::vector<Detection> postprocess(
        const std::vector<Tensor>& outputs,
        const PreprocessTransform& transform,
        const ModelSpec& spec,
        const InferenceOptions& options) = 0;
};
```

Responsibilities of adapters:

- model-specific preprocessing interpretation;
- raw-output decoding;
- model-specific coordinate format;
- distribution decoding when required;
- stride handling;
- confidence calculation;
- deciding whether external NMS is required;
- restoring coordinates to original image size.

Adapters must **not**:

- create Qt widgets;
- open cameras;
- own TensorRT/Paddle/ncnn runtime objects;
- write GUI code;
- contain backend-specific CUDA/ncnn/Paddle session logic.

Implement:

```text
NanoDetAdapter
PicoDetAdapter
YOLO26Adapter
```

---

# 11. Runtime Backend Interface

Create runtime-neutral backend API, for example:

```cpp
class IInferenceBackend
{
public:
    virtual ~IInferenceBackend() = default;

    virtual BackendInfo info() const = 0;

    virtual Status load(
        const ModelArtifact& artifact,
        const BackendConfig& config) = 0;

    virtual std::vector<Tensor> infer(
        const std::vector<Tensor>& inputs) = 0;

    virtual void unload() noexcept = 0;
};
```

Implement independent classes:

```text
NcnnBackend
OnnxRuntimeBackend
OpenVinoBackend
TensorRtBackend
PaddleBackend
```

The model adapter must not know which concrete backend executes the graph.

The backend must not know how NanoDet/PicoDet/YOLO26 bounding boxes are decoded.

---

# 12. Backend Capability Model

Each backend must report capabilities rather than requiring scattered `#ifdef` checks.

Example:

```text
backend
version
available devices
CPU support
GPU support
CUDA support
Vulkan support
FP32
FP16
INT8
dynamic shape
supported model format
```

Create a capability query so Desktop can disable unsupported choices instead of allowing invalid selections.

Example:

```text
TensorRT selected but no NVIDIA GPU
    -> UI shows backend unavailable
    -> do not crash
```

---

# 13. Runtime Strategy per Model Family

Use this as the initial preferred deployment strategy.

## NanoDet

Primary target:

```text
NanoDet
 -> ncnn
 -> CPU / ARM / optional Vulkan
```

Also allow:

```text
NanoDet -> ONNX Runtime
NanoDet -> OpenVINO
```

where an exported compatible model is provided.

Do not embed NanoDet Python training code into the C++ core.

Use the official NanoDet inference/deployment behavior only as a correctness reference.

## PicoDet

Primary target:

```text
PicoDet
 -> Paddle Inference
```

Optional compatible exported formats may later use:

```text
PicoDet -> ONNX Runtime
PicoDet -> OpenVINO
```

Do not assume all PicoDet exports have identical output tensor layouts.

Read the model registry/deployment metadata.

## YOLO26

Primary NVIDIA target:

```text
YOLO26
 -> ONNX
 -> TensorRT
 -> NVIDIA GPU
```

Fallback:

```text
YOLO26 -> ONNX Runtime
YOLO26 -> OpenVINO
```

ncnn may be supported only when the exported model format is compatible.

The adapter must support both YOLO26 post-processing modes described below.

---

# 14. YOLO26 End-to-End vs Traditional Mode

This requirement is critical.

YOLO26 default end-to-end detection can produce final detections in approximately this logical form:

```text
(batch, max_det, 6)

[x1, y1, x2, y2, confidence, class_id]
```

In this mode:

```text
NO external NMS
```

Only apply:

- confidence filtering;
- class filtering;
- coordinate restore/clamp;
- max detection rules as appropriate.

However, some exports/backends can use the traditional one-to-many head.

Traditional mode requires:

```text
decode raw predictions
 -> confidence filtering
 -> NMS
 -> coordinate restore
```

Therefore:

**Never determine NMS solely from the model name `YOLO26`.**

Store/export metadata such as:

```json
{
  "postprocess": {
    "mode": "yolo26_end2end",
    "requires_nms": false
  }
}
```

or:

```json
{
  "postprocess": {
    "mode": "yolo26_one_to_many",
    "requires_nms": true
  }
}
```

The adapter must choose the correct path from validated model metadata/output shape.

If actual model output is inconsistent with metadata, fail with a diagnostic error rather than interpreting arbitrary memory as detections.

---

# 15. NanoDet Algorithm Rules

NanoDet decoding is model-specific.

Do not invent constants because a particular demo used them.

Model metadata/config must be able to represent information required by the selected NanoDet variant, such as:

- input size;
- color conversion;
- mean/std or normalization;
- strides;
- regression distribution settings;
- class count;
- confidence threshold;
- NMS threshold;
- output tensor names/layout.

General pipeline:

```text
image
 -> resize/letterbox according to model config
 -> color/normalization
 -> tensor
 -> runtime
 -> NanoDet head decode
 -> confidence filtering
 -> NMS
 -> restore coordinates
 -> clamp
 -> detections
```

Validate results against reference inference on known test images.

---

# 16. PicoDet Algorithm Rules

PicoDet preprocessing/postprocessing must also be metadata-driven.

The model registry must be capable of representing:

- resize behavior;
- input shape;
- scale factors if required by the exported graph;
- normalization;
- channel order;
- output tensor names;
- class count;
- distribution/regression decoding parameters if outside the graph;
- NMS requirements.

General logical pipeline:

```text
image
 -> resize/preprocess
 -> prepare all required model inputs
 -> Paddle/runtime inference
 -> PicoDet output decode
 -> threshold
 -> NMS only when required by that export
 -> restore coordinates
 -> detections
```

Do not assume that a Paddle deployment model and an ONNX-converted PicoDet model expose identical input/output names.

---

# 17. Image Preprocessing

Create reusable preprocessing utilities.

Required features:

- BGR <-> RGB conversion;
- direct resize;
- aspect-ratio-preserving letterbox;
- normalization;
- channel reordering HWC -> CHW;
- FP32 and optionally FP16 input conversion;
- padding;
- transform metadata.

Create a transform object so postprocessing can correctly restore boxes.

Example:

```cpp
struct PreprocessTransform
{
    int originalWidth;
    int originalHeight;

    int networkWidth;
    int networkHeight;

    float scaleX;
    float scaleY;

    float padX;
    float padY;

    bool letterboxed;
};
```

Never reverse-resize a bounding box using guessed constants.

---

# 18. NMS

Implement reusable, unit-tested NMS in the common detection layer.

Support:

```text
class-aware NMS
class-agnostic NMS
```

Requirements:

- deterministic output ordering;
- IoU calculation validated by unit tests;
- configurable IoU threshold;
- configurable maximum detections;
- no NMS call for models explicitly marked end-to-end/NMS-free.

Do not run NMS twice when the model/runtime has already returned NMS-filtered detections.

---

# 19. Object/Class Selection

The Desktop allows users to choose which classes they want to display/detect, for example:

```text
[x] person
[x] car
[ ] bicycle
[ ] dog
```

For pretrained COCO models, do **not** reload or retrain the model when classes change.

Normal real-time behavior:

```text
model inference
 -> model decode
 -> model-required postprocessing
 -> selected class filter
 -> display
```

Class filtering is a logical output filter.

The framework may optimize earlier filtering only when it is proven not to change the model/postprocessing semantics.

Benchmark mode must clearly distinguish:

```text
full-model performance benchmark
```

from:

```text
user-filtered display results
```

Do not claim the neural network itself skipped unselected classes unless the actual exported graph supports that behavior.

---

# 20. Model Registry

Do not hard-code 16 model definitions throughout C++ source.

Create a model registry.

Recommended layout:

```text
models/
├── nanodet/
│   ├── nanodet-plus-m-320/
│   ├── nanodet-plus-m-416/
│   ├── nanodet-plus-m-1.5x-320/
│   └── nanodet-plus-m-1.5x-416/
│
├── picodet/
│   ├── picodet-s-320/
│   ├── picodet-s-416/
│   ├── picodet-m-320/
│   ├── picodet-m-416/
│   ├── picodet-l-320/
│   ├── picodet-l-416/
│   └── picodet-l-640/
│
└── yolo26/
    ├── yolo26n/
    ├── yolo26s/
    ├── yolo26m/
    ├── yolo26l/
    └── yolo26x/
```

Each model directory should contain metadata, not necessarily the large model binary.

Example:

```text
model.json
labels.txt
README.md
```

Artifacts may be located externally or ignored by Git.

---

# 21. Model Registry Schema

Use JSON unless there is a strong technical reason for another format.

Example:

```json
{
  "schema_version": 1,
  "id": "nanodet-plus-m-416",
  "display_name": "NanoDet-Plus-m-416",
  "family": "nanodet",
  "source_framework": "pytorch",

  "input": {
    "width": 416,
    "height": 416,
    "color_order": "BGR_TO_RGB",
    "layout": "CHW",
    "dtype": "FP32"
  },

  "preprocess": {
    "resize": "letterbox",
    "pad_value": 0,
    "scale": 0.003921568627,
    "mean": [0.0, 0.0, 0.0],
    "std": [1.0, 1.0, 1.0]
  },

  "labels": "labels.txt",

  "postprocess": {
    "decoder": "nanodet_plus",
    "confidence_threshold": 0.35,
    "iou_threshold": 0.60,
    "requires_nms": true
  },

  "artifacts": {
    "ncnn": {
      "param": "model.param",
      "bin": "model.bin"
    },
    "onnxruntime": {
      "model": "model.onnx"
    }
  }
}
```

The numeric values above are only an example of schema shape.

**Do not blindly use example preprocessing values.**

Populate each real model profile from the actual export/configuration used for that model.

Validate:

- missing keys;
- unsupported schema version;
- invalid dimensions;
- missing labels;
- artifact not found;
- backend not supported;
- output mode inconsistent with model metadata.

---

# 22. Model Artifact Rules

Do not commit large binary weights by default.

Add documentation explaining how users supply/export models.

The repository may include:

```text
models/**/model.json
models/**/labels.txt
```

while ignoring:

```text
*.onnx
*.engine
*.param
*.bin
*.pdmodel
*.pdiparams
*.pt
*.ckpt
```

Adjust ignore rules carefully so files that are genuinely required source/configuration files are not accidentally ignored.

If Git LFS is later used, document it explicitly.

---

# 23. Detector Factory

Create a clean factory/registry.

Concept:

```cpp
DetectorFactory::create(modelSpec, backendName)
```

Flow:

```text
ModelRegistry
  -> ModelSpec
  -> ModelAdapterRegistry chooses NanoDet/PicoDet/YOLO26 adapter
  -> BackendRegistry chooses runtime
  -> DetectorPipeline combines adapter + backend
```

Do not implement this using a huge switch statement in the Qt window.

Registration must occur below the GUI layer.

---

# 24. Complete Inference Pipeline

The core inference pipeline must follow this logic:

```text
1. Validate model and backend state
2. Validate input image
3. Start total timer
4. ModelAdapter preprocess
5. Record preprocess latency
6. Backend inference
7. Record inference latency
8. Validate output tensors
9. ModelAdapter postprocess
10. Apply required threshold/NMS logic
11. Restore bounding boxes to original image coordinates
12. Clamp invalid/out-of-image coordinates
13. Apply selected-class display filter
14. Apply max-detection rule
15. Record postprocess/total latency
16. Return DetectionResult + timing metadata
```

Failures must return useful diagnostics.

No uncaught backend exception should terminate the desktop application.

---

# 25. Model Loading / Switching

When switching models or backends:

```text
pause inference worker
 -> stop consuming old model
 -> release old runtime resources
 -> clear stale inference result
 -> load new backend/model
 -> validate input/output metadata
 -> run optional warmup
 -> atomically publish new detector
 -> resume inference
```

Do not retain a previous TensorRT/Paddle/ncnn model in GPU/RAM accidentally.

Do not allow results produced by the old model to be displayed as if they came from the newly selected model.

Associate results with:

```text
frame ID
model generation ID
model ID
```

and discard stale results.

---

# 26. TensorRT Rules

TensorRT is for NVIDIA GPU acceleration.

Requirements:

- use native C++ TensorRT APIs;
- manage CUDA stream/resources with RAII wrappers;
- validate binding/tensor names;
- validate tensor shapes and data types;
- do not assume static binding indexes if current TensorRT APIs expose named tensors;
- support FP32 first;
- add FP16 only when hardware/model supports it;
- INT8 only with an explicit calibration strategy;
- synchronize only when required;
- report CUDA/TensorRT errors clearly.

A serialized `.engine` is hardware/runtime specific.

Do not assume one TensorRT engine can safely be moved to every NVIDIA GPU or TensorRT version.

Preferred workflow:

```text
ONNX model
 -> TensorRT build for target hardware
 -> engine cache
```

The cache key should include enough information to avoid loading an incompatible engine, for example:

```text
model hash
TensorRT version
CUDA/runtime information
GPU identity/compute capability
precision
input shape
```

Implement engine caching only after correctness works without it.

---

# 27. ncnn Rules

ncnn is the preferred lightweight backend for ARM/CPU and optionally Vulkan.

Support:

```text
CPU
ARM optimizations provided by ncnn
optional Vulkan
```

Do not make Vulkan mandatory.

ncnn model paths must be supplied through model metadata.

Avoid unnecessary copies between OpenCV and ncnn tensors where safely possible, but correctness takes priority.

Do not put ncnn headers in public `odf_core` headers.

---

# 28. Paddle Inference Rules

Paddle backend must:

- load Paddle inference deployment artifacts;
- query/validate actual input names;
- query/validate output names;
- support CPU initially;
- support GPU only when compiled/configured with compatible Paddle/CUDA support;
- handle model-specific auxiliary inputs if required;
- expose backend capability information.

Paddle types must not leak into `odf_core` public API.

---

# 29. ONNX Runtime Rules

ONNX Runtime backend should provide the simplest portable baseline when available.

Support initially:

```text
CPU Execution Provider
```

Optionally:

```text
CUDA Execution Provider
```

if built/enabled.

Read actual input/output metadata.

Do not hard-code tensor names when the model registry or runtime can resolve them.

---

# 30. OpenVINO Rules

OpenVINO backend is an optional optimized CPU/iGPU path.

Keep OpenVINO-specific types private to that backend.

Support:

- model loading;
- device enumeration where possible;
- CPU execution first;
- runtime-selected supported device.

---

# 31. Desktop Application

Use Qt 6 Widgets unless the existing repository later establishes another Qt UI approach.

The Desktop must act as a client of the public framework APIs.

Recommended layout:

```text
+----------------------------------------------------------------+
| Object Detection Framework                                     |
+----------------------+-----------------------------------------+
| Source               |                                         |
| [Camera v]            |                                         |
| [Open Image]          |                                         |
|                      |           Detection View                |
| Framework             |                                         |
| [PyTorch v]           |       bounding boxes + labels          |
|                      |                                         |
| Model                 |                                         |
| [YOLO26n v]           |                                         |
|                      |                                         |
| Backend               |                                         |
| [TensorRT v]          |                                         |
|                      |                                         |
| Device                |                                         |
| [GPU 0 v]             |                                         |
|                      |                                         |
| Classes               |                                         |
| [x] person            |                                         |
| [x] car               |                                         |
| [ ] dog               |                                         |
+----------------------+-----------------------------------------+
| FPS | Pre | Infer | Post | Total | RAM | VRAM | Model         |
+----------------------------------------------------------------+
```

Required functionality:

- choose image file;
- choose camera;
- start/stop camera;
- select source framework grouping;
- select compatible model;
- select compatible backend;
- select device;
- select confidence threshold;
- select IoU threshold where relevant;
- choose classes;
- select all / clear all classes;
- draw bounding boxes and labels;
- show model/backend status;
- show live performance metrics;
- show errors non-destructively;
- allow model switching without restarting the app.

Do not expose unsupported backend/model combinations as if they work.

---

# 32. Qt Threading Rules

Never run heavy inference on the Qt GUI thread.

Use a pipeline similar to:

```text
Camera Capture Worker
        |
     latest frame
        |
Inference Worker
        |
Detection Result
        |
Qt GUI Thread
```

The GUI must remain responsive while:

- camera is running;
- model is loading;
- inference is running;
- model is switched;
- benchmark is running.

Use Qt signal/slot boundaries for GUI interaction.

Core inference should remain standard C++ and should not depend on Qt.

---

# 33. Real-Time Camera Queue

Do **not** use an unbounded frame queue.

For live camera mode, prioritize low latency over processing every frame.

Recommended:

```text
queue capacity = 1 or 2
```

Policy:

```text
if inference is slower than camera:
    discard stale frame
    keep newest frame
```

This prevents:

```text
camera latency = 1 s -> 2 s -> 5 s -> 20 s
```

Do not block the capture thread waiting for inference unless an explicit deterministic-recording mode is implemented.

Track:

```text
captured frames
processed frames
dropped frames
```

---

# 34. Image Mode

For image input:

```text
open image
 -> retain original
 -> inference
 -> render result on a copy
```

Never repeatedly draw on the original image because threshold/class changes would accumulate old bounding boxes.

When detection settings change, rerun or re-filter as appropriate.

---

# 35. Drawing

Create reusable visualization utilities.

Draw:

- box;
- class label;
- confidence;
- optional inference information.

Rendering must not modify detection coordinates.

Use a deterministic class-color mapping.

Do not put drawing logic inside model adapters.

---

# 36. Benchmark Engine

Benchmarking is a first-class feature of this project.

Separate:

```text
Performance Benchmark
Accuracy Benchmark
```

## Performance Benchmark

Measure independently:

```text
preprocess
inference
postprocess
total
```

Run:

```text
load
 -> warmup N iterations
 -> measured M iterations
 -> aggregate
```

Record:

- mean;
- median/p50;
- p95;
- p99;
- min/max;
- FPS;
- model size;
- backend;
- precision;
- device;
- input resolution.

Do not include model loading time in steady-state inference FPS.

Report model load time separately.

Use a monotonic clock.

Do not benchmark by updating the Qt UI inside the timed inference section.

---

# 37. Hardware Resource Benchmark

Where practical, collect:

```text
CPU utilization
process RAM
GPU utilization
VRAM
```

Implement platform-specific collectors behind an abstraction.

If a metric is unavailable, return:

```text
unsupported / unavailable
```

Do not fabricate `0` and present it as a measurement.

The benchmark system must still work if resource telemetry is unavailable.

---

# 38. Accuracy Benchmark

To choose a model based on accuracy, support evaluation against a labeled dataset.

Target metrics:

```text
Precision
Recall
mAP@0.50
mAP@0.50:0.95
```

Prefer COCO-compatible annotation input.

Keep accuracy evaluation outside the GUI thread.

Do not compare models only by visually inspecting one image.

If full COCO metric implementation is too large for the first milestone:

1. implement a clean evaluator interface;
2. implement correct IoU/AP primitives and tests;
3. support export of detections to COCO-compatible JSON;
4. clearly mark any external/reference evaluator step;
5. never label an incomplete approximation as official COCO mAP.

The final architecture must support a fully native evaluator later without changing detector APIs.

---

# 39. Benchmark Comparison

Create a result format that allows comparison like:

```text
Model                     Backend     Device       FPS   Infer ms   mAP
NanoDet-Plus-m-320        ncnn        ARM CPU      ...
PicoDet-S-320             Paddle      CPU          ...
YOLO26n                    TensorRT    NVIDIA GPU   ...
```

Do not compare published benchmark numbers from different upstream projects as if they were measured under identical hardware.

The primary comparison in this framework must use:

```text
same machine
same input/data
same benchmark policy
same warmup rules
same precision where possible
```

---

# 40. Model Recommendation Engine

Design the benchmark result format so a recommendation feature can be added.

Optional first implementation:

User specifies:

```text
minimum FPS
maximum RAM/VRAM
minimum accuracy
priority: speed / balanced / accuracy
```

Recommendation logic must be deterministic and explainable.

Example conceptual scoring:

```text
reject any model violating hard constraints

normalize remaining metrics

score =
    speedWeight * speedScore +
    accuracyWeight * accuracyScore +
    memoryWeight * memoryScore
```

Do not hard-code a claim such as "YOLO26 is always best".

Recommendation must be derived from measurements on the user's target hardware.

---

# 41. Framework-Core Reuse

The project must support future projects that do **not** use Qt.

Example PyTorch-origin future project:

```cmake
target_link_libraries(my_app
    PRIVATE
        odf_core
        odf_torch_core
        odf_backend_ncnn
)
```

or:

```cmake
target_link_libraries(my_app
    PRIVATE
        odf_core
        odf_torch_core
        odf_backend_tensorrt
)
```

Paddle project:

```cmake
target_link_libraries(my_app
    PRIVATE
        odf_core
        odf_paddle_core
        odf_backend_paddle
)
```

Public framework APIs must therefore have zero dependency on the desktop module.

---

# 42. Examples

Implement usable examples.

## `torch_core_example`

Demonstrate:

```text
load NanoDet or YOLO26 through odf_torch_core
select a runtime
infer one image
print detections
```

## `paddle_core_example`

Demonstrate:

```text
load PicoDet through odf_paddle_core
use Paddle backend
infer one image
print detections
```

## `image_detection`

Generic API only:

```text
registry -> detector factory -> detect image
```

It should not need knowledge of specific decoder classes.

## `camera_detection`

Generic real-time camera detection without Qt.

Use OpenCV display and low-latency latest-frame logic where appropriate.

---

# 43. Unit Tests

Use a C++ testing framework such as GoogleTest or Catch2.

At minimum test:

```text
BoundingBox
IoU
NMS
class filtering
coordinate restore
letterbox transform
model registry JSON parsing
invalid model metadata
backend capability selection
detector factory
YOLO26 end-to-end output parser
YOLO26 traditional-mode selection
stale result rejection
benchmark statistics
```

Add model-adapter fixture tests with small synthetic tensors.

For real model integration tests, make them optional because model files may not be included in Git.

Never require an NVIDIA GPU for ordinary core unit tests.

---

# 44. Integration Validation

For each supported real model profile, create a documented validation procedure.

Compare C++ output against the appropriate official/reference inference pipeline using the same:

```text
model weights
input image
resize/preprocessing
confidence threshold
IoU threshold
postprocessing mode
```

Validate:

```text
class IDs
confidence within tolerance
box coordinates within tolerance
detection count
```

Do not consider a backend complete merely because it loads a model without crashing.

---

# 45. Error Handling

Define project error categories, for example:

```text
InvalidArgument
FileNotFound
InvalidModelConfig
UnsupportedBackend
UnsupportedModelBackendPair
BackendUnavailable
ModelLoadFailure
InferenceFailure
TensorShapeMismatch
CameraFailure
DatasetFailure
```

Error message example:

Bad:

```text
Error
```

Good:

```text
TensorRT model load failed:
model=yolo26n
artifact=models/yolo26/yolo26n/model.engine
expected input=images
reason=engine was built with an incompatible TensorRT version
```

Never silently fall back to another model/backend without informing the caller.

Automatic fallback is allowed only if explicitly enabled in configuration and must be reported.

---

# 46. Logging

Implement a lightweight project logging layer.

Levels:

```text
Trace
Debug
Info
Warning
Error
```

Core code must not use `std::cout` everywhere.

Examples may print to console.

Desktop can route logs to:

- console;
- file;
- optional UI diagnostic panel.

Avoid logging every frame at `Info` level.

---

# 47. Configuration

Separate:

```text
model metadata
application preferences
backend configuration
benchmark configuration
```

Do not create one giant JSON file for everything.

Desktop settings may remember:

```text
last camera
last model
last backend
confidence threshold
selected classes
window settings
```

Do not store absolute developer-machine paths as defaults in committed files.

---

# 48. Device Selection

Expose devices generically.

Examples:

```text
CPU
CUDA:0
CUDA:1
Vulkan:0
OpenVINO:CPU
OpenVINO:GPU
Paddle:CPU
Paddle:GPU:0
```

A backend owns the mapping from generic selection to runtime-specific device handles.

Do not put CUDA device-management code in model adapters.

---

# 49. Precision

Represent:

```text
FP32
FP16
INT8
```

The UI/backend selection must expose only valid precision choices.

Rules:

- FP32 is baseline.
- FP16 only when backend/device/model supports it.
- INT8 requires a valid quantized model or calibration workflow.
- Never pretend a FP32 model became INT8 merely because the UI option changed.

---

# 50. Performance Rules

Optimize only after correctness.

Important practices:

- reuse allocated input/output buffers where safe;
- avoid reallocating large vectors every frame;
- avoid repeated model loading;
- avoid unnecessary `cv::Mat` deep copies;
- avoid host-device copies not required by the chosen backend;
- reserve detection containers when count is known;
- warm up GPU runtimes before benchmarking;
- do not optimize by removing validation that prevents unsafe tensor access.

Every optimization must preserve detection correctness.

---

# 51. Thread Safety

Clearly document whether a detector instance is:

```text
single-threaded
serialized
or safe for concurrent infer()
```

Do not assume third-party runtime sessions are safely callable from arbitrary concurrent threads.

For the first implementation, it is acceptable for one detector instance to have one inference worker.

Multiple parallel detector instances may be added later.

Protect model switching from concurrent inference.

---

# 52. Cancellation and Shutdown

Desktop shutdown must safely stop:

```text
camera
inference worker
benchmark worker
model loader
```

Then release:

```text
CUDA/TensorRT
Paddle
ncnn/OpenVINO/ONNX sessions
camera
Qt resources
```

No detached worker thread may access destroyed GUI/core objects.

Use cooperative cancellation.

---

# 53. License and Third-Party Rules

The repository already has a project license.

Do not replace it unless explicitly instructed.

For upstream code/models:

- inspect and respect each upstream license;
- do not copy large upstream source trees into this repository merely to make a demo compile;
- prefer linking official runtimes and implementing project-owned adapters;
- preserve required notices for copied/derived snippets;
- document third-party dependencies in `THIRD_PARTY_NOTICES.md` or equivalent;
- document model-license considerations separately from framework source-code licensing.

Especially for YOLO/Ultralytics assets, do not vendor upstream source into the core by default.

Prefer:

```text
user exports/provides deployment artifact
 -> ODF loads exported artifact
```

The C++ core should not require the Ultralytics Python package at runtime.

---

# 54. Security / Robustness

Treat model config and model files as external input.

Validate:

- JSON types;
- numeric ranges;
- shape sizes;
- arithmetic overflow when computing tensor sizes;
- negative/NaN/Inf outputs;
- file paths;
- camera indexes;
- image load failure.

Do not read/write beyond tensor buffer bounds even if metadata is wrong.

Limit unreasonable allocations derived from corrupt model metadata.

---

# 55. Documentation

Create or expand:

```text
README.md
docs/architecture.md
docs/build.md
docs/models.md
docs/backends.md
docs/benchmark.md
docs/adding_a_model.md
docs/adding_a_backend.md
THIRD_PARTY_NOTICES.md
```

README should explain:

1. what Object Detection Framework is;
2. supported model families;
3. supported backends;
4. architecture;
5. how to build minimal core;
6. how to build Desktop;
7. how to supply model artifacts;
8. simple C++ usage;
9. current feature matrix;
10. platform limitations.

Do not claim a model/backend is supported until a real inference path exists and has been tested.

Use feature states such as:

```text
Supported
Experimental
Planned
Unavailable
```

---

# 56. Adding a New Model

Document the intended extension workflow.

A new model should normally require:

```text
1. add ModelAdapter
2. register adapter
3. add model metadata
4. add tests
```

It should **not** require editing:

```text
Qt MainWindow
camera source
benchmark engine
every backend
common detection data structures
```

This is a major architecture acceptance criterion.

---

# 57. Adding a New Backend

A new backend should normally require:

```text
1. implement IInferenceBackend
2. add CMake integration
3. expose capability information
4. register backend
5. add tests
```

It should not require rewriting NanoDet/PicoDet/YOLO26 adapters unless the backend produces a fundamentally different validated export format.

---

# 58. Initial Model Matrix

Create initial metadata/config entries for exactly these 16 profiles:

## NanoDet

```text
nanodet-plus-m-320
nanodet-plus-m-416
nanodet-plus-m-1.5x-320
nanodet-plus-m-1.5x-416
```

## PicoDet

```text
picodet-s-320
picodet-s-416
picodet-m-320
picodet-m-416
picodet-l-320
picodet-l-416
picodet-l-640
```

## YOLO26

```text
yolo26n
yolo26s
yolo26m
yolo26l
yolo26x
```

Do not invent downloadable weight URLs or unsupported conversion parameters.

If artifacts are not included, provide metadata templates and exact documented steps for users to place valid exported artifacts.

---

# 59. Platform Targets

Design for:

## Desktop

```text
Windows x64
Linux x64
```

## Edge

Architecturally support:

```text
Linux ARM64
Raspberry Pi class devices
```

through lightweight backends such as ncnn.

## NVIDIA

Support:

```text
NVIDIA desktop GPU
Jetson class devices
```

through TensorRT where compatible.

Do not require all platforms to compile every backend.

Platform/backend support must be selected through CMake.

---

# 60. CMake Dependency Policy

Preferred order:

1. `find_package(...)` for installed/configured dependencies.
2. Optional documented `ODF_*_ROOT` hints.
3. FetchContent only for small reasonable dependencies where licensing and network behavior are appropriate.

Do not automatically download multi-gigabyte CUDA/Paddle/TensorRT packages during ordinary CMake configure.

Provide actionable error messages.

Example:

```text
ODF_ENABLE_TENSORRT=ON but TensorRT was not found.
Set TensorRT_ROOT or disable ODF_ENABLE_TENSORRT.
```

---

# 61. Public Headers

Keep public include tree clean.

Example:

```text
include/odf/
├── detection/
├── pipeline/
├── model/
├── backend/
├── benchmark/
└── version.hpp
```

Do not expose:

```text
NvInfer.h
cuda_runtime.h
net.h from ncnn
paddle_inference_api.h
Qt headers
```

through `odf_core` public headers.

Use PImpl/private implementation where helpful.

---

# 62. ABI / API Future Readiness

Do not over-engineer a stable binary plugin ABI in the first version.

However:

- keep backend/model interfaces clean;
- avoid backend-specific types in common interfaces;
- isolate implementation details;
- design so dynamic plugins can be added later.

Document API version:

```text
ODF_VERSION_MAJOR
ODF_VERSION_MINOR
ODF_VERSION_PATCH
```

---

# 63. Suggested Implementation Milestones

Implement in this order.

## Milestone 1 — Foundation

Create:

```text
CMake structure
odf_core
types
status/error system
logging
model registry
backend/model adapter interfaces
tests
```

Acceptance:

```text
core builds with no heavy inference runtime installed
unit tests pass
```

## Milestone 2 — Generic Pipeline

Implement:

```text
preprocessing utilities
coordinate transforms
NMS
DetectorPipeline
factory/registry
```

Acceptance:

```text
synthetic model/backend tests pass
```

Do not create a fake backend exposed to end users.

A mock backend is allowed **only inside tests**.

## Milestone 3 — NanoDet + ncnn

Implement real:

```text
NanoDetAdapter
NcnnBackend
NanoDet inference
```

Validate one profile first, then all requested NanoDet profiles.

## Milestone 4 — PicoDet + Paddle

Implement:

```text
PicoDetAdapter
PaddleBackend
PicoDet inference
```

Validate all requested profiles.

## Milestone 5 — YOLO26 + ONNX Runtime

Implement portable YOLO26 first:

```text
YOLO26Adapter
OnnxRuntimeBackend
end-to-end output mode
traditional output mode
```

## Milestone 6 — YOLO26 + TensorRT

Implement NVIDIA acceleration.

Validate:

```text
model loading
shape handling
FP32
FP16 optional
engine lifecycle
```

## Milestone 7 — OpenVINO

Add portable CPU/iGPU option.

## Milestone 8 — Desktop

Build Qt UI against the already validated libraries.

## Milestone 9 — Benchmark

Add performance and resource benchmarking.

## Milestone 10 — Accuracy / Recommendation

Add dataset evaluation and hardware/model recommendation.

---

# 64. Do Not Build the GUI First

This is mandatory.

Do not start by constructing a polished Qt interface around unimplemented inference.

Correct order:

```text
Core API
 -> Model Registry
 -> Model Adapters
 -> Backends
 -> CLI/examples
 -> tests/reference validation
 -> Desktop UI
```

The project must remain useful without Qt.

---

# 65. Definition of Done

The project is not considered complete merely because it compiles.

Minimum completion criteria:

## Architecture

- `odf_core` is model/runtime independent.
- `odf_torch_core` is reusable independently of Qt.
- `odf_paddle_core` is reusable independently of Qt.
- backend implementation is separate from model adapters.

## Models

- registry contains all 16 requested profiles.
- at least one validated real deployment path exists for each model family.
- no fake detection path exists in production code.

## Runtime

- model/backend compatibility is validated.
- errors are descriptive.
- resource cleanup is correct.

## Desktop

- image detection works.
- camera detection works.
- model switching works.
- class selection works.
- GUI stays responsive.
- real metrics are displayed.

## Benchmark

- warmup and measurement phases are separated.
- preprocess/inference/postprocess timings are separated.
- benchmark result can be persisted/exported.
- unavailable telemetry is explicitly marked unavailable.

## Tests

- core unit tests pass.
- NMS/IoU/coordinate transformations have tests.
- model-output parser tests exist.
- optional integration tests are documented.

## Documentation

- build instructions are reproducible.
- supported/experimental/planned status is accurate.
- adding a model/backend is documented.
- third-party license notes exist.

---

# 66. Code Quality Rules

While implementing:

1. Prefer small cohesive classes.
2. Avoid files containing thousands of unrelated lines.
3. Do not duplicate preprocessing or NMS code across adapters.
4. Do not create `Utils.cpp` as a dumping ground.
5. Keep model-specific constants in model metadata or adapter-local definitions.
6. Add assertions only for programmer invariants; use runtime validation for external data.
7. Use `const` correctness.
8. Make ownership explicit.
9. Avoid macros except build/platform integration.
10. Do not suppress compiler warnings globally.
11. Enable useful warnings for project-owned code.
12. Do not treat warnings from third-party headers as project errors when unavoidable.
13. Format code consistently.
14. Keep the code easy to review.

---

# 67. AI Working Rules

When executing this task in the repository:

- First inspect the current tree, README, license, CMake files, and any existing source.
- Do not assume the repository is empty.
- Reuse valid existing code.
- Do not change unrelated functionality.
- Make changes in logical milestones.
- Compile/test after each meaningful stage when the environment permits.
- If a heavy backend cannot be compiled because its SDK is unavailable, still complete its clean CMake integration and interface implementation only when it can be made truthful; otherwise mark it disabled/planned rather than adding code that pretends to work.
- Never report a backend/model as tested without actually testing it.
- Never fabricate benchmark numbers.
- Never fabricate accuracy numbers.
- Never fabricate model files.
- Never silently download unknown binaries.
- Never hard-code paths from the developer's local computer.
- Never commit credentials, tokens, or machine-specific secrets.
- Explain any unavoidable limitation in documentation.

When encountering uncertainty in an upstream model's tensor layout, preprocessing, export behavior, or required NMS:

```text
inspect the official model/export configuration
 -> inspect actual runtime tensor metadata
 -> implement explicit validated handling
 -> add a test
```

Do not guess.

---

# 68. Upstream Technical References

Use upstream projects/documentation as behavioral references, while respecting their licenses.

## NanoDet

Official repository:

```text
https://github.com/RangiLyu/nanodet
```

Relevant concepts:

- NanoDet-Plus model configs;
- official C++ ncnn deployment example;
- OpenVINO demo;
- real preprocessing/postprocessing behavior.

## PaddleDetection / PicoDet

Official repository:

```text
https://github.com/PaddlePaddle/PaddleDetection
```

Relevant concepts:

- PicoDet model zoo;
- deployment export configuration;
- Paddle Inference C++ deployment;
- preprocessing/decode metadata.

## Ultralytics YOLO26

Official documentation:

```text
https://docs.ultralytics.com/models/yolo26/
https://docs.ultralytics.com/guides/end2end-detection/
https://docs.ultralytics.com/modes/export/
```

Important:

- YOLO26 default end-to-end mode can be NMS-free.
- Traditional export mode may require NMS.
- Do not assume all export backends preserve the same output mode.

## ncnn

```text
https://github.com/Tencent/ncnn
```

## TensorRT

Use the NVIDIA TensorRT C++ documentation matching the installed version.

## Paddle Inference

Use the official Paddle Inference C++ documentation matching the deployed version.

---

# 69. Final Deliverable Required From the AI

After implementation, provide a concise engineering report containing:

```text
1. Final source tree
2. Libraries/targets created
3. Models actually supported
4. Backend/model compatibility matrix
5. Files added/modified
6. Build commands
7. Test commands and results
8. Real inference tests performed
9. Known limitations
10. Remaining planned work
```

Do not merely say "implemented successfully".

Include exact information that allows another developer to reproduce the build.

---

# 70. Desired Final Architecture

The final codebase should conceptually look like:

```text
                        +----------------------+
                        |      Qt Desktop      |
                        +----------+-----------+
                                   |
                                   v
                        +----------------------+
                        |       ODF Core       |
                        |----------------------|
                        | Detection Types      |
                        | Pipeline             |
                        | Camera / Image       |
                        | Benchmark            |
                        | Model Registry       |
                        +----+-------------+---+
                             |             |
                +------------+             +-------------+
                v                                        v
      +--------------------+                   +--------------------+
      |   ODF Torch Core   |                   |  ODF Paddle Core   |
      |--------------------|                   |--------------------|
      | NanoDet Adapter    |                   | PicoDet Adapter    |
      | YOLO26 Adapter     |                   +--------------------+
      +----------+---------+
                 |
                 +------------------+
                                    |
                                    v
                      +-----------------------------+
                      |       Runtime API           |
                      | IInferenceBackend           |
                      +-------------+---------------+
                                    |
          +-------------------------+--------------------------+
          |              |              |            |         |
          v              v              v            v         v
       +------+      +---------+     +--------+   +------+  +--------+
       | ncnn |      | ONNX RT |     |OpenVINO|   |TRT   |  |Paddle  |
       +--+---+      +----+----+     +---+----+   +--+---+  +---+----+
          |               |              |           |          |
       CPU/ARM          CPU/CUDA        CPU/iGPU   NVIDIA     CPU/GPU
       Vulkan
```

The most important design objective is:

> **Adding a model must not require rewriting a backend.  
> Adding a backend must not require rewriting the Desktop.  
> Reusing a framework core must not require Qt.**

Build the project around that rule.
