# Object Detection Framework

Object Detection Framework (ODF) 0.1.0 là SDK C++17 và ứng dụng Desktop Windows MVP, tách bộ
chuyển đổi theo từng họ model khỏi runtime suy luận. Luồng triển khai đã được xác thực là:

```text
YOLO26n -> ONNX Runtime 1.26.0 (CPU/FP32) -> pipeline ODF
         -> OpenCV đọc ảnh/camera -> Desktop Qt 6 Widgets
```

## Trạng thái hiện tại

| Hạng mục | Trạng thái |
|---|---|
| Pipeline lõi, registry, tiền xử lý, NMS, API benchmark | Đã cài đặt và có unit test |
| Adapter YOLO26 và NanoDet | Đã cài đặt; có test giải mã tensor tổng hợp |
| Adapter PicoDet | Đã cài đặt; có test giải mã tensor tổng hợp |
| Backend ONNX Runtime | Đã cài đặt cho CPU/FP32 |
| Desktop Qt, CLI ảnh, CLI camera | MVP đã cài đặt |
| Backend ncnn, OpenVINO, TensorRT, Paddle Inference | Đang lên kế hoạch; bật cờ sẽ dừng cấu hình với lỗi rõ ràng |
| Model registry | 16 profile: một YOLO26n đã xác thực và 15 template bị khóa mặc định |

File `models/yolo26/yolo26n/model.onnx` hiện có và SHA-256 khớp hồ sơ validation. Trọng số sinh ra
bị Git ignore và CMake không tự tải chúng. Source C++ có thể build sạch, nhưng cần cung cấp trước
các native dependency và artifact model nếu muốn chạy suy luận thật.

## Build sạch trên Windows

Chỉ dùng `build-desktop/` cho cấu hình Desktop:

```powershell
cmake -S . -B build-desktop `
  -G "Visual Studio 17 2022" -A x64 `
  -DODF_BUILD_DESKTOP=ON `
  -DODF_BUILD_TESTS=ON `
  -DODF_BUILD_EXAMPLES=ON `
  -DODF_ENABLE_ONNXRUNTIME=ON `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.11.2\msvc2022_64" `
  -DOpenCV_DIR="C:\SDK\opencv-4.14.0\opencv\build\x64\vc16\lib" `
  -DONNXRUNTIME_ROOT="C:\SDK\onnxruntime-1.26.0"

cmake --build build-desktop --config Release --parallel
ctest --test-dir build-desktop -C Release --output-on-failure
```

Executable Desktop nằm tại:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe
```

Quá trình build đặt Qt, OpenCV, ONNX Runtime, metadata registry, nhãn và các artifact model đang có
cạnh executable.

## Chạy chương trình

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime
```

Xem [mục lục tài liệu tiếng Việt](docs/Vn/README.md) để đọc hướng dẫn đầy đủ, kiến trúc, thuật
toán, quy tắc validation và xử lý sự cố.

## Phạm vi giấy phép

Source ODF tuân theo [`LICENSE`](LICENSE) của repository. SDK runtime và artifact model giữ giấy
phép riêng; xem [thông báo bên thứ ba](THIRD_PARTY_NOTICES_Vn.md).
