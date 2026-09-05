# Thông báo bên thứ ba

Repository được theo dõi bởi Git chứa source C++ thuộc project và danh sách tên class COCO. Nó
không đóng gói source framework upstream, runtime SDK hoặc trọng số đã huấn luyện. Các file model
sinh cục bộ có thể tồn tại trong working tree bị ignore và được copy vào output build/deploy cũng
bị ignore.

Các tích hợp tùy chọn tham chiếu đến những project có giấy phép độc lập:

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

`model.onnx` đã xác thực cục bộ được export bằng Ultralytics 8.4.138 và khai báo giấy phép
Ultralytics AGPL-3.0 trong metadata ONNX. Model cùng source `.pt` bị Git ignore và không được
repository này cấp lại giấy phép.

Người dùng phải kiểm tra giấy phép đúng với từng phiên bản được cài đặt hoặc phân phối. Giấy phép
model/checkpoint có thể khác giấy phép runtime và source. Việc export hoặc deploy không chuyển
giấy phép của tác giả model sang ODF và không mặc nhiên biến artifact thành GPL-3.0.
