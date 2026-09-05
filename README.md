# Object Detection Framework

[English](README/README_En.md) | [Tiếng Việt](README/README_Vn.md)

Choose a language above. Detailed documentation is organized under [`docs/En`](docs/En/README.md)
and [`docs/Vn`](docs/Vn/README.md).

Chọn ngôn ngữ ở trên. Tài liệu chi tiết được tổ chức trong [`docs/En`](docs/En/README.md)
và [`docs/Vn`](docs/Vn/README.md).

## Desktop IDE / Application Generator dành cho Object Detection

```Structure
              OBJECT DETECTION FRAMEWORK

                        ODF Desktop
                            │
          ┌─────────────────┼─────────────────┐
          │                 │                 │
       INPUT             MODEL            HARDWARE
          │                 │                 │
       Camera             YOLO             CPU
       Video              RTMDet           CUDA
       Image              PicoDet          TensorRT
       RTSP               NanoDet          OpenVINO
                         Custom ONNX
          │                 │
          └─────────────────┼─────────────────┘
                            │
                        CONFIGURE
                            │
                  Confidence / IoU
                  Classes / Input Size
                  Pre/Post processing
                            │
                            ▼
                       LIVE TEST
                            │
                            ▼
                      BENCHMARK
                            │
                            ▼
                   ★ GENERATE PROJECT ★
                            │
              ┌─────────────┼──────────────┐
              ▼             ▼              ▼
             C++          Python        Embedded
              │
         ONNX Runtime
         OpenVINO
         TensorRT
```
### While ODF could take the direction of

```Structure
ODF

Visual Configuration
       ↓
Model + Backend + Hardware
       ↓
Test / Benchmark
       ↓
Generate
       ↓
Pure C++ Project
       ↓
No ODF required
```

- ODF excels in:

```
Model
 ↓
Configuration
 ↓
Runtime
 ↓
Hardware benchmark
 ↓
Deployment
 ↓
Source generation
```