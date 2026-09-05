# YOLO26n ONNX validation record

This record applies only to the exact artifact hash below. If `model.onnx` is replaced, set
`deployment_validated=false` until the comparison is repeated.

| Field | Value |
|---|---|
| Artifact | `model.onnx` |
| SHA-256 | `4f9a606c410c6c5b4bdc887f80aaebb05ae37c0d7487dace14182768508da234` |
| Ultralytics version | `8.4.138` (`torch 2.14.0+cpu`) |
| ONNX version/opset | `1.22.0`, `ai.onnx:18` |
| Export command | `.venv-export\Scripts\python.exe scripts\export_yolo26n_onnx.py` |
| Input | `images`, `[1,3,640,640]`, FP32 |
| Output | `output0`, `[1,300,6]`, FP32; metadata `end2end=True`, `nms=False` |
| Runtime | C++ ONNX Runtime `1.26.0`, CPU/FP32 |
| Reference image | Ultralytics package `assets/bus.jpg`, 810×1080 |
| Confidence / IoU | `0.25` / `0.45` |
| Ultralytics ONNX result | 5 objects: 1 bus, 4 persons |
| ODF C++ result | 5 objects: 1 bus, 4 persons; historical validation CLI total `43.31 ms` |
| Comparison | Class/count exact; maximum coordinate delta `0.263 px`; maximum confidence delta `0.0032` |
| Camera validation | Not part of artifact equivalence; test per device |

The latency is evidence from that validation run, not a portable benchmark. Generated weights and
ONNX artifacts are ignored by Git. Review their upstream license separately from ODF's source
license.
