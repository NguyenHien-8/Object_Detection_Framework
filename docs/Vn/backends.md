# Backend

| Backend | Tùy chọn CMake | Build target | Trạng thái | Device/precision |
|---|---|---|---|---|
| ONNX Runtime | `ODF_ENABLE_ONNXRUNTIME` | `odf_backend_onnxruntime` | MVP đã cài đặt | CPU/FP32 |
| ncnn | `ODF_ENABLE_NCNN` | — | Dự kiến; cấu hình từ chối ON | — |
| OpenVINO | `ODF_ENABLE_OPENVINO` | — | Dự kiến; cấu hình từ chối ON | — |
| TensorRT | `ODF_ENABLE_TENSORRT` | — | Dự kiến; cấu hình từ chối ON | — |
| Paddle Inference | `ODF_ENABLE_PADDLE` | — | Dự kiến; cấu hình từ chối ON | — |

`IInferenceBackend` báo cáo đúng availability, version, device, precision, format, hỗ trợ dynamic
shape, CUDA và Vulkan. `DetectorFactory` kiểm tra cặp model/backend được yêu cầu. Backend không
khả dụng sẽ không được hiển thị như đã dùng được và không bị thay ngầm bằng runtime khác.

Backend ONNX Runtime kiểm tra SDK khi cấu hình CMake. Khi load, nó tạo session thật, đọc tên và
element type của tensor rồi đối chiếu profile, chỉ chấp nhận CPU/FP32 đã khai báo. Khi infer, nó
kiểm tra tensor input FP32 liên tục có tên, chạy graph thật và copy output FP32 hợp lệ sang
`Tensor` trung lập của ODF. Thiếu file, sai device/precision, sai tên/type/số lượng, output không
phải tensor hoặc exception runtime đều được trả về bằng lỗi có phân loại.

Tuple đã xác thực là YOLO26n + ONNX Runtime 1.26.0 + CPU/FP32, input `images`
`[1,3,640,640]`, output `output0` `[1,300,6]`.
