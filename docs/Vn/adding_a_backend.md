# Thêm backend

1. Cài đặt `IInferenceBackend` trong `backends/<name>` và ẩn header vendor trong source/PImpl.
2. Thêm tùy chọn CMake opt-in, dùng `find_package` hoặc biến cache chỉ SDK root có tài liệu. Cấu
   hình thông thường không được tự tải SDK lớn.
3. Báo đúng device, precision, format, availability và khả năng tăng tốc bằng `BackendInfo`.
4. Khi load, kiểm tra artifact và tên, type, số lượng, shape input/output thật của session.
5. Nhận tensor trung lập có tên của ODF và trả output thật; backend không giải mã box model.
6. Chỉ đăng ký với `RuntimeCatalog` khi target backend đã được compile.
7. Stage runtime library cần thiết, thêm test với SDK thật và ghi phiên bản/dependency deploy đã
   xác thực.

Không fallback ngầm, không để type vendor rò vào `odf_core`, không trả detection giả và không báo
thành công trước khi runtime thật sự thực thi.
