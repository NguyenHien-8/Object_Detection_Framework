# Third-party notices

The tracked repository contains project-owned C++ source and COCO class-name text. It does not
vendor upstream framework source, runtime SDKs, or pretrained weights. Locally generated model
files may exist in an ignored working tree and may be copied into ignored build/deployment output.

Optional integrations refer to independently licensed projects:

- NanoDet: <https://github.com/RangiLyu/nanodet>
- PaddleDetection / PicoDet: <https://github.com/PaddlePaddle/PaddleDetection>
- Ultralytics YOLO26: <https://github.com/ultralytics/ultralytics>
- ONNX Runtime: <https://github.com/microsoft/onnxruntime>
- ncnn: <https://github.com/Tencent/ncnn>
- OpenVINO: <https://github.com/openvinotoolkit/openvino>
- NVIDIA TensorRT: <https://developer.nvidia.com/tensorrt>
- Paddle Inference: <https://www.paddlepaddle.org.cn/inference/master/index.html>
- Qt: <https://www.qt.io/licensing>
- OpenCV: <https://opencv.org/license/>

The locally validated `model.onnx` was exported with Ultralytics 8.4.138 and declares the
Ultralytics AGPL-3.0 license in its ONNX metadata. The model and its `.pt` source are ignored by
Git and are not relicensed by this repository.

Users must review the license matching each exact version they install or distribute. A model or
checkpoint license may differ from runtime and source licenses. Exporting or deploying a model
does not transfer the model author's license to ODF and does not make the artifact GPL-3.0 by
default.

The repository [`LICENSE`](LICENSE) remains unchanged.
