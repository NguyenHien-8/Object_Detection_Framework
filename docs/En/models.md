# Models and artifacts

## Registry inventory

The repository contains exactly 16 checked-in `model.json` profiles:

- NanoDet: `nanodet-plus-m-320`, `nanodet-plus-m-416`,
  `nanodet-plus-m-1.5x-320`, `nanodet-plus-m-1.5x-416`;
- PicoDet: `picodet-s-320`, `picodet-s-416`, `picodet-m-320`, `picodet-m-416`,
  `picodet-l-320`, `picodet-l-416`, `picodet-l-640`;
- YOLO26: `yolo26n`, `yolo26s`, `yolo26m`, `yolo26l`, `yolo26x`.

Only `yolo26n` is marked `deployment_validated=true`, for the exact ONNX artifact recorded in its
validation file. The other 15 entries are metadata templates and are rejected during normal load.

## Current YOLO26n artifact

The runtime-declared file is:

```text
models/yolo26/yolo26n/model.onnx
```

Expected SHA-256:

```text
4f9a606c410c6c5b4bdc887f80aaebb05ae37c0d7487dace14182768508da234
```

Verify it with:

```powershell
Get-FileHash -Algorithm SHA256 `
  -LiteralPath .\models\yolo26\yolo26n\model.onnx
```

The artifact is ignored by Git. Deleting `build-desktop/` is safe because the source copy remains
under `models/`; deleting both copies removes the model and requires export or restoration. CMake
copies artifacts but never creates or downloads them.

## What a profile controls

Each `model.json` supplies model identity/family, class count and labels, input name/resolution/
color/layout/type, resize and normalization rules, postprocess mode, NMS policy, strides,
`reg_max`, supported backend names, default backend, and backend-specific input/output/artifact
paths. These values are executable contracts, not descriptive hints.

A profile may list a future backend artifact such as OpenVINO or TensorRT even though that runtime
is not compiled in this release. The runtime catalog still exposes only implemented, available
backends.

## Supplying or replacing a model

1. Pin an upstream release/commit and checkpoint.
2. Export with that upstream project's documented deployment workflow.
3. Put files beside `model.json`, or change the profile's relative artifact paths.
4. Inspect real input/output names, ranks, shapes, and element types.
5. Make preprocessing and postprocessing metadata match the exported graph.
6. Compare ODF output with the upstream reference using
   [integration validation](integration_validation.md).
7. Record artifact hash, tool versions, input, thresholds, and tolerances.
8. Set `deployment_validated=true` only for that exact artifact/profile tuple.

For YOLO26n, the provided export and inspection helpers are:

```powershell
.\.venv-export\Scripts\python.exe .\scripts\export_yolo26n_onnx.py
.\.venv-export\Scripts\python.exe .\scripts\inspect_onnx.py `
  .\models\yolo26\yolo26n\model.onnx
```

Python is an export/validation dependency, not a dependency of the final C++ executables.
