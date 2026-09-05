# Đo hiệu năng

`runPerformanceBenchmark` nhận một detector đã load và một ảnh. Các vòng warmup chạy trước và
không được tính. Vòng đo ghi riêng thời gian preprocess, inference, postprocess và tổng thời gian
từ đồng hồ đơn điệu của pipeline.

Kết quả gồm mean, min, max, p50, p95, p99, FPS, model/backend/device/precision, độ phân giải input,
số vòng, kích thước artifact tùy chọn, RAM/VRAM tùy chọn và timestamp. `writeBenchmarkJson` ghi
telemetry chưa có thành JSON `null`, không giả thành số 0.

Giới hạn hiện tại:

- chưa có bộ thu thập RAM và VRAM;
- chưa có đánh giá accuracy và export COCO JSON;
- kết quả sinh ra thuộc `benchmarks/results/` và bị Git ignore;
- chỉ so sánh khi cùng phần cứng, chế độ nguồn, runtime, artifact, input, warmup, threshold và
  precision;
- hủy benchmark theo cơ chế hợp tác giữa các vòng lặp.

Không nên so sánh trực tiếp FPS hiển thị một lần trên Desktop với benchmark có kiểm soát, vì render
UI và capture camera nằm ngoài lời gọi detector được đo.
