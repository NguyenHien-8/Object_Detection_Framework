# Object Detection Framework

Object Detection Framework (ODF) is a C++17 SDK foundation for composing object-detection
model adapters with independent inference runtimes. The common API, preprocessing, detection
types, NMS, registry, pipeline, and benchmarking code have no Qt or vendor-runtime dependency.

Generated model weights are ignored by Git. The local YOLO26n ONNX artifact has been exported,
inspected, and compared against Ultralytics; its exact hash is recorded in
[`VALIDATION.md`](models/yolo26/yolo26n/VALIDATION.md). Production code refuses to load the other
unvalidated template profiles unless the explicit developer override is used.

## Current status (0.1.0)

| Component | State | Evidence / limitation |
|---|---|---|
| `odf_core` | Supported | Builds with MSVC 19.44; unit-tested without external SDKs |
| `odf_torch_core` | Experimental | NanoDet DFL and YOLO26 parsers pass synthetic tensor tests; real weights not supplied |
| `odf_paddle_core` | Experimental | Paddle graph-postprocessed PicoDet parser passes synthetic tests; real weights not supplied |
| ONNX Runtime CPU backend | Supported MVP | ONNX Runtime 1.26.0; real YOLO26n CPU/FP32 load and inference test passes |
| ncnn / OpenVINO / TensorRT / Paddle backends | Planned | Enabling an unimplemented option fails configuration explicitly |
| Qt desktop / OpenCV camera and image clients | Supported MVP | Qt 6.11.2/OpenCV 4.14.0 build and desktop smoke test pass; camera code is hardware-dependent |
| 16 registry profiles | 1 validated, 15 templates | YOLO26n is validated for the recorded artifact; all other profiles remain guarded |

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
Neither operation requires changes to the Qt window. See [architecture](docs/architecture.md).

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

## Build and run the desktop MVP on Windows

```powershell
cmake -S . -B build-desktop -G "Visual Studio 17 2022" -A x64 `
  -DODF_BUILD_DESKTOP=ON -DODF_ENABLE_ONNXRUNTIME=ON `
  -DCMAKE_PREFIX_PATH=C:\Qt\6.11.2\msvc2022_64 `
  -DOpenCV_DIR=C:\SDK\opencv-4.14.0\opencv\build\x64\vc16\lib `
  -DONNXRUNTIME_ROOT=C:\SDK\onnxruntime-1.26.0
cmake --build build-desktop --config Release --parallel
build-desktop\desktop\Qt\Release\odf_desktop.exe --models models
```

The executable directory is staged with the OpenCV/ONNX Runtime DLLs, Qt runtime/plugins, and
the model registry. See [YOLO26n quickstart](docs/yolo26n_quickstart.md) for export, validation,
CLI image inference, desktop use, and troubleshooting.

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

Loading is allowed only for a profile whose validation record matches its local artifact.
Follow [models.md](docs/models.md) before replacing YOLO26n or enabling another profile.

## Documentation

- [Build and test](docs/build.md)
- [Architecture and thread-safety](docs/architecture.md)
- [Model profiles and artifact workflow](docs/models.md)
- [Backend capability matrix](docs/backends.md)
- [Benchmark semantics](docs/benchmark.md)
- [Adding a model](docs/adding_a_model.md)
- [Adding a backend](docs/adding_a_backend.md)
- [Real-model validation procedure](docs/integration_validation.md)
- [Qt desktop guide](docs/desktop.md)
- [Windows dependencies](docs/windows_dependencies.md)
- [YOLO26n quickstart](docs/yolo26n_quickstart.md)

ODF source is licensed under the repository's existing GPL-3.0 license. Model and runtime
licenses are separate; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
