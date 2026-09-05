# Validation tích hợp với model thật

Nạp được graph chỉ chứng minh tương thích, chưa chứng minh detection đúng. Mỗi tuple model/profile/
backend/device/precision phải được xác thực riêng.

1. Ghi release/commit upstream, hash checkpoint, hash artifact export, lệnh export, phiên bản
   runtime, device và precision.
2. Dùng một input cố định và cùng color conversion, resize, normalization, confidence, IoU, class
   filter, số detection tối đa và postprocess ở ODF và upstream.
3. Ghi tên, type và shape tensor do runtime báo trước khi decode.
4. So sánh chính xác số detection và class ID; sau đó so confidence và tọa độ `xyxy` trên ảnh gốc
   theo sai số đã công bố.
5. Lưu bằng chứng tái tạo được và thêm real-model test dạng opt-in. Test thông thường không phụ
   thuộc download, GPU hoặc webcam.
6. Lặp lại với profile anh em và kiểu export khác; validation không tự chuyển giữa các artifact.

## Bằng chứng hiện tại

Artifact YOLO26n cục bộ có SHA-256
`4f9a606c410c6c5b4bdc887f80aaebb05ae37c0d7487dace14182768508da234` đã được so sánh bằng ảnh
tham chiếu Ultralytics `bus.jpg`, confidence `0.25`, IoU `0.45`. Cả hai đường đều trả một bus và
bốn person; class/số lượng khớp hoàn toàn, sai lệch tọa độ lớn nhất `0.263 px`, sai lệch confidence
lớn nhất `0.0032`. Xem [hồ sơ artifact tiếng Việt](../../models/yolo26/yolo26n/VALIDATION_Vn.md).

`validation-output/` bị Git ignore và có thể xóa. Hồ sơ validation vẫn được giữ, nhưng nếu thay
`model.onnx` thì phải so sánh lại và đặt `deployment_validated=false` cho đến khi đạt.

Validation camera là bài kiểm tra riêng theo thiết bị. Bộ test thông thường không tuyên bố đã bao
phủ camera vật lý.
