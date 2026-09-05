# Hướng dẫn nhanh YOLO26n trên Windows

Đây là đường ngắn nhất được hỗ trợ từ source đến Desktop MVP đã xác thực.

## 1. Kiểm tra model

```powershell
Test-Path -LiteralPath .\models\yolo26\yolo26n\model.onnx
Get-FileHash -Algorithm SHA256 `
  -LiteralPath .\models\yolo26\yolo26n\model.onnx
```

Hash mong đợi là
`4f9a606c410c6c5b4bdc887f80aaebb05ae37c0d7487dace14182768508da234`. Nếu thiếu file, export
trong môi trường Python chỉ dùng cho phát triển:

```powershell
py -3.12 -m venv .venv-export
.\.venv-export\Scripts\python.exe -m pip install ultralytics onnx onnxruntime
.\.venv-export\Scripts\python.exe .\scripts\export_yolo26n_onnx.py
.\.venv-export\Scripts\python.exe .\scripts\inspect_onnx.py `
  .\models\yolo26\yolo26n\model.onnx
```

Hợp đồng mong đợi: input `images` FP32 `[1,3,640,640]`; output `output0` FP32 `[1,300,6]`;
metadata `end2end=True`, `nms=False`. Hash mới chưa được xác thực cho đến khi so sánh tham chiếu đạt.

## 2. Cấu hình, build và test

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

## 3. Kiểm tra registry

```powershell
.\build-desktop\examples\Release\odf_registry_inspect.exe .\models
```

Kết quả cần có 16 profile và `yolo26n` ở trạng thái `validated`. Cờ này là điều kiện cần; lệnh hash
ở bước trên mới ràng buộc cờ với artifact hiện tại.

## 4. Chạy suy luận ảnh bằng CLI

```powershell
.\build-desktop\examples\image_detection\Release\odf_image_detection.exe `
  --models .\models `
  --model yolo26n `
  --backend onnxruntime `
  --image "C:\path\to\input.jpg" `
  --output "C:\path\to\result.jpg"
```

CLI in thời gian thật của từng bước và các detection; trả mã khác 0 khi ảnh thiếu/lỗi, thiếu
artifact, tensor không đúng hợp đồng hoặc runtime thất bại.

## 5. Chạy Desktop

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime
```

Model được nạp trong worker thread. Mở ảnh hoặc khởi động camera trên UI. Cũng có thể mở trực tiếp
một ảnh:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime `
  --image "C:\path\to\input.jpg"
```

## 6. Chạy CLI camera

```powershell
.\build-desktop\examples\camera_detection\Release\odf_camera_detection.exe `
  --models .\models --model yolo26n --backend onnxruntime --camera 0
```

Nhấn Esc hoặc Q để dừng. Nếu index 0 không dùng được, đóng ứng dụng khác đang giữ camera, kiểm tra
quyền riêng tư Windows và thử index khác. CTest đạt không đồng nghĩa camera sẽ mở được.

## 7. Xử lý sự cố

- Thiếu `Qt6Config.cmake`: trỏ `CMAKE_PREFIX_PATH` đến Qt MSVC2022 x64.
- Thiếu `OpenCVConfig.cmake`: trỏ `OpenCV_DIR` đến thư mục chứa file đó.
- Thiếu ONNX header/library/DLL: dùng SDK C/C++ chính thức, không dùng Python package.
- Thiếu DLL hoặc Qt platform plugin: build lại Desktop target hoặc chạy script deploy.
- Thiếu `model.onnx`: export hoặc khôi phục dưới đúng thư mục profile.
- Artifact bị từ chối vì chưa validation: so sánh đúng hash và output; không dùng developer
  override lâu dài như giải pháp deploy.
- Sai shape: kiểm tra graph; end-to-end cần `[1,300,6]`, không phải `[1,84,8400]`.
- Không thấy box: kiểm tra confidence và class selection; xóa toàn bộ class thì đúng là không vẽ box.
- Box lệch: kiểm tra letterbox căn giữa, làm tròn resize và phép biến đổi tọa độ ngược.
- Sai màu: ảnh nguồn là BGR và bước tiền xử lý theo profile phải đổi sang RGB.
