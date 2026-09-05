# Thêm model

1. Cài đặt `IModelAdapter` trong thư viện framework phù hợp; không đưa kiểu runtime hoặc UI vào.
2. Định nghĩa một hợp đồng tensor rõ ràng và từ chối rank, shape, type, giá trị hoặc miền class sai.
3. Đăng ký adapter creator qua hàm đăng ký framework được `RuntimeCatalog` sử dụng.
4. Thêm `model.json` có version và file/tham chiếu nhãn dưới `models/<family>/<profile>/`.
5. Thêm test decoder tổng hợp, test hợp đồng lỗi, test preprocess/tọa độ và quy trình tham chiếu thật.
6. Test qua `DetectorFactory` để kiểm tra cả việc ghép adapter/backend.
7. Giữ `deployment_validated=false` đến khi so sánh với upstream đã chốt đạt cho đúng hash artifact.

Giải mã riêng của model chỉ thuộc adapter. Detection chung, camera, benchmark, backend và Qt widget
không nên cần nhánh điều kiện theo họ model.

Nếu output artifact khác hợp đồng của họ model hiện có, hãy thêm postprocess mode rõ ràng cùng test;
không suy đoán theo tên file hoặc kích thước quan sát được lúc chạy.
