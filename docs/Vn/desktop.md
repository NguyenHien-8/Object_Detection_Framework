# Ứng dụng Desktop Qt

`odf_desktop` là client Qt 6 Widgets của public pipeline ODF. UI không chứa decoder model và
widget không dùng trực tiếp kiểu dữ liệu ONNX Runtime.

## Khởi động ứng dụng

Từ thư mục gốc repository:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime
```

Mở ngay một ảnh:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime `
  --image "C:\path\to\image.jpg"
```

Khởi động camera sau khi model được nạp:

```powershell
.\build-desktop\desktop\Qt\Release\odf_desktop.exe `
  --models .\models --model yolo26n --backend onnxruntime --camera 0
```

## Cách tìm model root

Resolver chung thử theo thứ tự: `--models` được truyền trực tiếp, biến môi trường
`ODF_MODEL_ROOT`, `<thư-mục-executable>/models`, rồi thư mục model đã cài đặt. Desktop còn thử
model root đã lưu và mở hộp chọn thư mục nếu không tìm thấy registry dùng được. Khi chẩn đoán lỗi
khởi động, truyền đường dẫn tuyệt đối cho `--models` là cách xác định nhất.

## Quy trình sử dụng

1. Kiểm tra Framework, Model, Backend, CPU và FP32 rồi bấm **Load model**. Thay combobox chưa đổi
   detector đang nạp cho đến khi bấm nút này.
2. Bấm **Refresh** để phục hồi trạng thái nguồn và quét lại camera mà thông thường không reload model.
3. Bấm **Open image…**, hoặc chọn tên camera thật rồi bấm **Start camera**.
4. Chỉnh confidence, IoU và số detection tối đa. Với graph YOLO26n end-to-end, NMS đã trong graph
   vì metadata là `requires_nms=false`, nên IoU không chạy NMS lần hai ở ODF.
5. Tìm class, chọn tất cả, xóa tất cả hoặc bật/tắt từng class. Không chọn class nào thì giao diện
   chủ động không vẽ box nào.
6. Theo dõi thời gian preprocess, inference, postprocess, tổng thời gian, FPS, số object và số
   frame bị bỏ ở bảng metrics.

Lọc class chỉ vẽ lại detection đã cache, không chạy graph lại. Viewport giữ tỷ lệ ảnh nguồn và chỉ
ánh xạ tọa độ ảnh gốc lúc vẽ, nên resize cửa sổ không làm thay đổi kết quả suy luận.

## Trạng thái và lỗi

Controller duy trì trạng thái model và state machine camera có acknowledgement:

```text
Idle -> Opening -> Running -> Stopping -> Idle
          \            \               -> Error
```

Nút Start bị khóa trong Opening; nút hiện `Stopping...` và bị khóa đến khi worker xác nhận release.
Nếu yêu cầu Load model khi camera đang hoạt động, controller chờ acknowledgement đó rồi mới reload.
Thiếu artifact, ảnh lỗi, tensor không tương thích, camera không khả dụng và exception runtime đều
hiện ở vùng status, không tạo detection giả.

Refresh giữ nguyên model đã nạp. Khi camera đang chạy, nó dừng/release session cũ, vô hiệu source
generation đang chờ hoặc in-flight, quét thiết bị, khôi phục đúng stable ID và chờ frame đầu tiên
của session mới. Khi camera dừng, nó chỉ xóa trạng thái nguồn cũ và quét thiết bị. Trong image mode,
nó gửi lại ảnh đang giữ đúng một lần.

`QSettings` lưu model/backend, symbolic link ổn định của camera, threshold, class selection, thư mục ảnh gần nhất,
model root và hình học cửa sổ.

## Tham số dòng lệnh

```text
--models PATH
--model ID
--backend ID
--image PATH | --camera N
--output PATH
--confidence N
--iou N
--max-detections N
--log-level LEVEL
--allow-unvalidated-model
```

`--allow-unvalidated-model` chỉ dành cho kiểm thử phát triển có chủ đích. Nó chỉ bỏ qua cờ registry,
không bỏ qua kiểm tra tensor, file còn thiếu hoặc khả năng backend.

## Đóng chương trình và lưu ý camera

Khi đóng cửa sổ, app dừng capture, join camera thread, dừng và join inference rồi unload detector.
Camera còn phụ thuộc thiết bị, driver và quyền riêng tư; CTest đạt không chứng minh webcam vật lý
hoạt động. Dùng executable test phần cứng trong [Build và kiểm thử](build.md) để kiểm tra restart lặp.
