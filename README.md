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
- For example, a student or an engineer without specialized AI expertise opens an ODF.
```example
Model:
YOLO

Model:
yolo26n.onnx

Device:
NVIDIA RTX 3050

Backend:
ONNX Runtime CUDA

Input:
Logitech C920

Objects:
☑ Person
☐ Bicycle
☑ Car
☐ Motorcycle

Confidence:
0.55

IoU:
0.45

Resolution:
640 × 640
```
- Click Test
```
FPS             54.7
Inference       13.8 ms
Preprocess       1.7 ms
Postprocess      1.1 ms
CPU             18 %
GPU             42 %
RAM            580 MB
VRAM           735 MB
```

- If that works: Generate C++ Project
+ ODF Generate:
```
Person_Car_Detector/
│
├── CMakeLists.txt
├── README.md
│
├── config/
│   └── detector.yaml
│
├── models/
│   └── yolo26n.onnx
│
├── include/
│   ├── detector.hpp
│   └── camera.hpp
│
└── src/
    ├── main.cpp
    ├── detector.cpp
    └── camera.cpp
```
- Users do not need to build the object detection source code from scratch; they simply need to configure the models and generation settings, then run the project's source code.

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