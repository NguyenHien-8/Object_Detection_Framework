# Tài liệu ODF — Tiếng Việt

Nên bắt đầu từ [Build và test](build.md), sau đó đọc [hướng dẫn nhanh YOLO26n](yolo26n_quickstart.md).

| Tài liệu | Nội dung |
|---|---|
| [Kiến trúc và thuật toán](architecture.md) | Cấu trúc source, phụ thuộc, luồng suy luận và luồng đa luồng |
| [Build và test](build.md) | Cấu hình CMake, output, kiểm thử, cài đặt |
| [Dependency Windows](windows_dependencies.md) | Bố trí native SDK và đóng gói |
| [Ứng dụng Desktop](desktop.md) | Thao tác UI, trạng thái model/nguồn, threading, tham số CLI |
| [Model và artifact](models.md) | Profile registry, quy tắc artifact, cách tìm model root |
| [Backend](backends.md) | Ma trận khả năng đúng với phần đã cài đặt |
| [Hướng dẫn nhanh YOLO26n](yolo26n_quickstart.md) | Quy trình build, test và chạy từ đầu đến cuối |
| [Validation tích hợp](integration_validation.md) | Cách xác thực đúng artifact và bằng chứng hiện có |
| [Benchmark](benchmark.md) | Ngữ nghĩa phép đo và giới hạn |
| [Thêm model](adding_a_model.md) | Checklist mở rộng adapter và registry |
| [Thêm backend](adding_a_backend.md) | Checklist mở rộng runtime |

Prompt gốc ở thư mục cha được giữ làm đầu vào lịch sử. Source, CMake target, test và bộ tài liệu
này mới là nguồn vận hành hiện tại.
