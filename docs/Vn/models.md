# Model và artifact

## Danh sách registry

Repository có đúng 16 profile `model.json`:

- NanoDet: `nanodet-plus-m-320`, `nanodet-plus-m-416`,
  `nanodet-plus-m-1.5x-320`, `nanodet-plus-m-1.5x-416`;
- PicoDet: `picodet-s-320`, `picodet-s-416`, `picodet-m-320`, `picodet-m-416`,
  `picodet-l-320`, `picodet-l-416`, `picodet-l-640`;
- YOLO26: `yolo26n`, `yolo26s`, `yolo26m`, `yolo26l`, `yolo26x`.

Chỉ `yolo26n` có `deployment_validated=true` cho đúng artifact ONNX được ghi trong hồ sơ
validation. 15 profile còn lại chỉ là template metadata và bị từ chối khi nạp theo mặc định.

## Artifact YOLO26n hiện tại

File được runtime khai báo là:

```text
models/yolo26/yolo26n/model.onnx
```

SHA-256 mong đợi:

```text
4f9a606c410c6c5b4bdc887f80aaebb05ae37c0d7487dace14182768508da234
```

Kiểm tra bằng:

```powershell
Get-FileHash -Algorithm SHA256 `
  -LiteralPath .\models\yolo26\yolo26n\model.onnx
```

Artifact bị Git ignore. Xóa `build-desktop/` là an toàn vì bản gốc còn trong `models/`; nếu xóa
cả hai bản thì phải export hoặc khôi phục model. CMake chỉ copy artifact, không tạo hay tải nó.

## Vai trò của profile

Mỗi `model.json` khai báo định danh/họ model, số class và nhãn, tên/kích thước/màu/layout/kiểu input,
quy tắc resize và normalize, chế độ postprocess, chính sách NMS, strides, `reg_max`, tên backend hỗ
trợ, backend mặc định và đường dẫn input/output/artifact theo backend. Đây là hợp đồng thực thi,
không chỉ là thông tin mô tả.

Profile có thể liệt kê artifact cho backend tương lai như OpenVINO hoặc TensorRT dù runtime đó
chưa được cài đặt trong bản hiện tại. Runtime catalog vẫn chỉ công bố backend thật sự đã compile
và khả dụng.

## Cung cấp hoặc thay model

1. Chốt release/commit upstream và checkpoint.
2. Export bằng quy trình deployment chính thức của upstream đó.
3. Đặt file cạnh `model.json` hoặc sửa đường dẫn artifact tương đối trong profile.
4. Kiểm tra tên, rank, shape và element type thật của input/output.
5. Đồng bộ metadata preprocess/postprocess với graph đã export.
6. So sánh output ODF với upstream theo [validation tích hợp](integration_validation.md).
7. Ghi hash artifact, phiên bản công cụ, input, threshold và sai số cho phép.
8. Chỉ đặt `deployment_validated=true` cho đúng tuple artifact/profile đã đạt.

Với YOLO26n, có sẵn helper export và kiểm tra:

```powershell
.\.venv-export\Scripts\python.exe .\scripts\export_yolo26n_onnx.py
.\.venv-export\Scripts\python.exe .\scripts\inspect_onnx.py `
  .\models\yolo26\yolo26n\model.onnx
```

Python chỉ là dependency cho export/validation, không cần để chạy executable C++ cuối cùng.
