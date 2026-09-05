# Hồ sơ validation ONNX YOLO26n

Hồ sơ này chỉ áp dụng cho đúng hash artifact dưới đây. Nếu thay `model.onnx`, phải đặt
`deployment_validated=false` cho đến khi chạy lại phép so sánh.

| Trường | Giá trị |
|---|---|
| Artifact | `model.onnx` |
| SHA-256 | `4f9a606c410c6c5b4bdc887f80aaebb05ae37c0d7487dace14182768508da234` |
| Phiên bản Ultralytics | `8.4.138` (`torch 2.14.0+cpu`) |
| Phiên bản/opset ONNX | `1.22.0`, `ai.onnx:18` |
| Lệnh export | `.venv-export\Scripts\python.exe scripts\export_yolo26n_onnx.py` |
| Input | `images`, `[1,3,640,640]`, FP32 |
| Output | `output0`, `[1,300,6]`, FP32; metadata `end2end=True`, `nms=False` |
| Runtime | ONNX Runtime C++ `1.26.0`, CPU/FP32 |
| Ảnh tham chiếu | Gói Ultralytics `assets/bus.jpg`, 810×1080 |
| Confidence / IoU | `0.25` / `0.45` |
| Kết quả Ultralytics ONNX | 5 object: 1 bus, 4 person |
| Kết quả ODF C++ | 5 object: 1 bus, 4 person; tổng thời gian CLI lịch sử `43.31 ms` |
| So sánh | Class/số lượng khớp; lệch tọa độ tối đa `0.263 px`; lệch confidence tối đa `0.0032` |
| Validation camera | Không thuộc phép tương đương artifact; cần test theo từng thiết bị |

Con số thời gian chỉ là bằng chứng của lần validation đó, không phải benchmark dùng chung. Trọng
số và ONNX artifact bị Git ignore. Cần xem giấy phép upstream riêng với giấy phép source ODF.
