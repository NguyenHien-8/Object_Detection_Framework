# Object Detection Framework

Object Detection Framework (ODF) is a C++17 SDK foundation for composing object-detection
model adapters with independent inference runtimes. The common API, preprocessing, detection
types, NMS, registry, pipeline, and benchmarking code have no Qt or vendor-runtime dependency.

The repository intentionally does not ship model weights. A registry profile is not considered
deployable until its exact exported artifact, tensor names/layout, preprocessing, and reference
output have been validated. Production code refuses to load unvalidated template profiles.

## Current status (0.1.0)

| Component | State | Evidence / limitation |
|---|---|---|
| `odf_core` | Supported | Builds with MSVC 19.44; unit-tested without external SDKs |
| `odf_torch_core` | Experimental | NanoDet DFL and YOLO26 parsers pass synthetic tensor tests; real weights not supplied |
| `odf_paddle_core` | Experimental | Paddle graph-postprocessed PicoDet parser passes synthetic tests; real weights not supplied |
| ONNX Runtime CPU backend | Experimental | Real runtime implementation exists; SDK/model were unavailable for integration testing |
| ncnn / OpenVINO / TensorRT / Paddle backends | Planned | Enabling an unimplemented option fails configuration explicitly |
| Qt desktop / OpenCV camera and image clients | Planned | Deliberately gated until a real model path is validated |
| 16 registry profiles | Template | Parse and validate; `deployment_validated=false` prevents false-positive loading |

“Experimental” above does not mean that real model accuracy has been validated. There are no
fabricated detections, benchmark results, or silent runtime fallbacks.

## Architecture

```text
applications
    -> DetectorFactory / DetectorPipeline (odf_core)
       -> IModelAdapter (odf_torch_core or odf_paddle_core)
       -> IInferenceBackend (separate backend library)
```

Adding a model registers a new adapter and metadata. Adding a backend registers a runtime.
Neither operation requires changes to a future Qt window. See [architecture](docs/architecture.md).

## Build the verified SDK

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DODF_BUILD_TESTS=ON -DODF_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Linux uses the same options with a compiler generator such as Ninja. No dependency is downloaded
during configuration. Full instructions and backend options are in [build.md](docs/build.md).

Run the registry inspector:

```powershell
build\examples\Release\odf_registry_inspect.exe models
```

For a single-config generator, omit `Release` from the executable path.

## Public API composition

```cpp
odf::model::ModelRegistry registry;
auto status = registry.loadDirectory("models");

odf::pipeline::DetectorFactory factory;
odf::models::registerTorchAdapters(factory);
factory.registerBackend("onnxruntime", [] {
    return std::make_unique<odf::backends::OnnxRuntimeBackend>();
});

const auto* spec = registry.find("yolo26n");
auto detector = factory.create(*spec, "onnxruntime");
```

Loading will still be rejected while the checked-in profile remains a template. Follow
[models.md](docs/models.md) to supply and validate an artifact before changing that state.

## Documentation

- [Build and test](docs/build.md)
- [Architecture and thread-safety](docs/architecture.md)
- [Model profiles and artifact workflow](docs/models.md)
- [Backend capability matrix](docs/backends.md)
- [Benchmark semantics](docs/benchmark.md)
- [Adding a model](docs/adding_a_model.md)
- [Adding a backend](docs/adding_a_backend.md)
- [Real-model validation procedure](docs/integration_validation.md)

ODF source is licensed under the repository's existing GPL-3.0 license. Model and runtime
licenses are separate; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
