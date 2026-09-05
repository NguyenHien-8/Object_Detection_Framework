# Build và kiểm thử

## Yêu cầu

- CMake 3.24 trở lên
- Visual Studio 2022 với bộ công cụ C++ x64
- Qt 6.5 trở lên, bản MSVC2022 x64, có Widgets
- OpenCV 4.x x64 với `core`, `imgproc`, `imgcodecs`, `videoio` và `highgui`
- SDK ONNX Runtime C/C++ CPU x64 chính thức
- Artifact model cục bộ để suy luận thật; quá trình build không tự tải model

Đặt toàn bộ output Desktop trong `build-desktop/`. Thư mục này bị Git ignore và có thể xóa mà
không làm thay đổi source hoặc artifact model gốc.

## Cấu hình

Chạy tại thư mục gốc repository trong PowerShell. Dấu backtick phải là ký tự cuối cùng của mỗi
dòng cần nối.

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
```

Hãy thay bằng đường dẫn thật trên máy đang dùng. `ONNXRUNTIME_ROOT` phải chứa C++ header, import
library và runtime DLL; Python wheel không thay thế được SDK này.

## Build Release

```powershell
cmake --build build-desktop --config Release --parallel
```

Generator Visual Studio là multi-configuration nên cả build và test đều cần `--config Release`.
Các output chính:

| Chương trình | Đường dẫn tính từ gốc repository |
|---|---|
| Desktop | `build-desktop\desktop\Qt\Release\odf_desktop.exe` |
| CLI ảnh | `build-desktop\examples\image_detection\Release\odf_image_detection.exe` |
| CLI camera | `build-desktop\examples\camera_detection\Release\odf_camera_detection.exe` |
| Kiểm tra registry | `build-desktop\examples\Release\odf_registry_inspect.exe` |
| Test | `build-desktop\tests\Release\odf_*_tests.exe` |

Tìm executable Desktop mà không cần đoán đường dẫn configuration:

```powershell
Get-ChildItem -LiteralPath .\build-desktop -Recurse -Filter odf_desktop.exe |
  Select-Object FullName, LastWriteTime
```

Bước post-build của Desktop gọi `windeployqt` nếu tìm thấy, copy ONNX Runtime, OpenCV Release DLL
và copy `models/` cạnh executable. Vì vậy artifact YOLO26n đã khai báo cần xuất hiện tại:

```text
build-desktop/desktop/Qt/Release/models/yolo26/yolo26n/model.onnx
```

## Kiểm thử

```powershell
ctest --test-dir build-desktop -C Release --output-on-failure
```

Cấu hình Desktop chuẩn tạo năm test:

1. `odf_unit_tests`: registry, tiền xử lý, hình học, NMS, adapter, benchmark cơ bản;
2. `odf_app_tests`: CLI/model-root/runtime catalog và tích hợp OpenCV;
3. `odf_desktop_tests`: cầu nối Qt và class filter ở chế độ offscreen;
4. `odf_desktop_smoke`: khởi động headless với registry bình thường;
5. `odf_desktop_missing_model_smoke`: xử lý lỗi thiếu artifact khi chạy headless.

Các test này không yêu cầu webcam và không chạy graph YOLO26n thật, trừ khi cấu hình thêm
`ODF_ENABLE_REAL_MODEL_TESTS=ON`, `ODF_TEST_MODEL_ROOT` hợp lệ và `ODF_TEST_IMAGE` là ảnh thật.
Cấu hình tùy chọn đó thêm test `odf_real_yolo26n`.

`odf_camera_hardware_test` được build cùng Desktop test nhưng chủ động không đăng ký vào CTest.
Chỉ chạy thủ công trên máy có camera khả dụng. Chương trình quét thiết bị Media Foundation thật,
lặp 20 chu kỳ `open -> frame đầu tiên -> acknowledgement release` và kiểm tra stable identity qua
một lần refresh giữa chừng:

```powershell
.\build-desktop\tests\Release\odf_camera_hardware_test.exe
```

Truyền thêm vị trí bắt đầu từ 0 trong danh sách mà utility in ra để kiểm tra thiết bị khác.

## Các tùy chọn chưa hỗ trợ

`ODF_ENABLE_NCNN`, `ODF_ENABLE_OPENVINO`, `ODF_ENABLE_TENSORRT` và `ODF_ENABLE_PADDLE` hiện dừng
cấu hình với lỗi giải thích rõ; chúng không sinh runtime giả. `ODF_ENABLE_CUDA` và
`ODF_ENABLE_VULKAN` không biến ONNX Runtime hiện tại thành backend GPU; đường đã xác thực chỉ là
CPU/FP32.

## Build sạch lại

Chỉ khi thực sự muốn cấu hình sạch, xóa đúng thư mục build rồi chạy lại configure/build/test:

```powershell
Remove-Item -LiteralPath .\build-desktop -Recurse -Force
```

Không xóa `models/yolo26/yolo26n/model.onnx` nếu chưa có cách sinh hoặc khôi phục. Artifact model
bị Git ignore.

## Cài đặt SDK

```powershell
cmake --install build-desktop --config Release --prefix C:\odf
```

Lệnh cài đặt export thư viện ODF đã build, public header, CMake package, metadata và nhãn, cộng
`odf_desktop` khi được bật. Để tạo thư mục Desktop có thể phân phối cùng native DLL, dùng script
deploy thay vì chỉ dùng `cmake --install`.
