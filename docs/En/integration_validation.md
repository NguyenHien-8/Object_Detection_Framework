# Real-model integration validation

Loading a graph proves compatibility, not detection correctness. Validate every model/profile/
backend/device/precision tuple independently.

1. Record upstream release/commit, checkpoint hash, exported artifact hash, export command,
   runtime version, device, and precision.
2. Use a fixed input and identical color conversion, resize, normalization, confidence, IoU,
   class filter, maximum detections, and postprocess mode in ODF and the upstream reference.
3. Record runtime tensor names, types, and shapes before decoding.
4. Compare detection count and class IDs exactly, then confidence and original-image `xyxy`
   coordinates with documented tolerances.
5. Store reproducible evidence and add an opt-in real-model test. Keep ordinary tests independent
   of downloads, GPU hardware, and webcams.
6. Repeat for sibling profiles and other exports; validation does not transfer between artifacts.

## Current validated evidence

The exact local YOLO26n artifact with SHA-256
`4f9a606c410c6c5b4bdc887f80aaebb05ae37c0d7487dace14182768508da234` was compared using the
Ultralytics `bus.jpg` reference at confidence `0.25` and IoU `0.45`. Both paths returned one bus
and four persons; class/count matched exactly, maximum coordinate delta was `0.263 px`, and
maximum confidence delta was `0.0032`. See the
[English artifact record](../../models/yolo26/yolo26n/VALIDATION_En.md).

`validation-output/` is intentionally ignored and may be deleted. The validation record remains,
but replacing `model.onnx` requires running the comparison again and setting
`deployment_validated=false` until it passes.

Camera validation is separate and device-specific. The normal test suite does not claim physical
camera coverage.
