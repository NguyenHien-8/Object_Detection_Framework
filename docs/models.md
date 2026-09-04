# Models and artifacts

## Registry inventory

Exactly 16 checked-in profiles are provided:

- NanoDet: `nanodet-plus-m-320`, `nanodet-plus-m-416`,
  `nanodet-plus-m-1.5x-320`, `nanodet-plus-m-1.5x-416`
- PicoDet: `picodet-s-320`, `picodet-s-416`, `picodet-m-320`,
  `picodet-m-416`, `picodet-l-320`, `picodet-l-416`, `picodet-l-640`
- YOLO26: `yolo26n`, `yolo26s`, `yolo26m`, `yolo26l`, `yolo26x`

YOLO26n is locally validated for the exact artifact hash in
`models/yolo26/yolo26n/VALIDATION.md`. The remaining 15 profiles are metadata templates with
`deployment_validated=false`; the pipeline refuses to load them by default. The known input
resolutions and baseline family configuration are not a substitute for inspecting an artifact.

## Supplying a model

1. Choose one exact upstream release/commit and record it.
2. Export the model using that upstream project's official deployment workflow.
3. Place the artifact beside its `model.json` using the filenames declared under `artifacts`, or
   update the relative paths. Large artifacts are ignored by Git.
4. Inspect actual runtime input/output names, ranks, shapes, and element types.
5. Update color order, resize policy, normalization, auxiliary inputs, output names, postprocess
   mode, strides, and `reg_max` to match that artifact.
6. Compare its C++ detections to the upstream reference using
   [integration_validation.md](integration_validation.md).
7. Only after comparison passes, set `deployment_validated` to `true` for that local artifact.

Do not commit a locally validated metadata flag without also documenting the artifact hash,
export version, and validation evidence.

## Family-specific placement

### NanoDet

Use the official NanoDet config and deployment/export tools matching the checkpoint. An ncnn
pair belongs at:

```text
models/nanodet/<profile>/model.param
models/nanodet/<profile>/model.bin
```

The adapter expects a concatenated NanoDet-Plus DFL head. If an exporter emits multiple feature
maps or a different layout, do not set the validation flag; adapt metadata/code and add a fixture
first.

### PicoDet

Export a Paddle inference model using PaddleDetection's version-matched `tools/export_model.py`.
Place its program/parameter files as declared by the profile. Check the generated deployment
configuration: input names and whether NMS is inside the graph vary by export. The present adapter
contract is graph-postprocessed `bbox` plus optional `bbox_num`; other layouts remain planned.

### YOLO26

For the default ONNX end-to-end head, use the checked-in helper:

```text
python scripts/export_yolo26n_onnx.py
python scripts/inspect_onnx.py models/yolo26/yolo26n/model.onnx
```

The helper puts the result at `models/yolo26/yolo26n/model.onnx`. End-to-end must yield
`[1, max_det, 6]`; one-to-many export uses `end2end=False`, yields
`[1, 4+class_count, predictions]`, and requires changing metadata to
`yolo26_one_to_many` with `requires_nms=true`. Never infer NMS behavior from the filename.

Model weights and generated artifacts retain their own licenses; supplying an artifact is the
user's responsibility.
