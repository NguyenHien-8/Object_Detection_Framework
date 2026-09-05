# Kiến trúc và thuật toán

ODF tách ngữ nghĩa model, runtime suy luận, tầng ứng dụng và giao diện thành các lớp độc lập. Chiều
phụ thuộc hướng vào các interface C++17 không gắn với runtime cụ thể.

## Cấu trúc source

```text
include/odf/                 public header của SDK
core/                        ảnh, detection, registry, pipeline, benchmark, frame buffer
frameworks/torch/            adapter model NanoDet và YOLO26
frameworks/paddle/           adapter model PicoDet
backends/onnxruntime/        backend ONNX Runtime CPU/FP32 đã cài đặt
backends/{ncnn,...}/         placeholder và tài liệu cho backend dự kiến
applications/                phần ghép chung cho CLI/Desktop và cầu nối OpenCV
desktop/Qt/                  UI Qt 6 Widgets, controller, settings, worker
examples/                    executable registry, ảnh và camera
models/                      model.json, nhãn và artifact sinh cục bộ
scripts/                     export, kiểm tra, so sánh tham chiếu, đóng gói Windows
tests/                       test core, application, Desktop, smoke và real-model tùy chọn
docs/{En,Vn}/                tài liệu vận hành tách theo ngôn ngữ
```

## Target build và quan hệ phụ thuộc

```text
odf_desktop / CLI ảnh / CLI camera
              |
              v
       odf_app_support --------------------> OpenCV
              |
              v
      DetectorFactory + DetectorPipeline
          |                     |
          v                     v
 IModelAdapter             IInferenceBackend
 (YOLO26/NanoDet/PicoDet)  (ONNX Runtime)
          \                     /
           +------ odf_core ---+
```

`odf_core` không include Qt, OpenCV hoặc header của vendor runtime. Adapter giữ quy tắc giải mã
tensor; backend chỉ nạp và thực thi graph. UI chỉ dùng `IDetector`, không có detector hoặc decoder
thứ hai.

## Registry và khởi tạo detector

Khi khởi động, `ModelRegistry` quét đệ quy model root để tìm `model.json`. Mỗi profile được kiểm
tra schema; đường dẫn tương đối đến nhãn/artifact được giải theo thư mục của profile.
`RuntimeCatalog` đăng ký adapter và backend đã được compile. `DetectorFactory` kiểm tra lần lượt:

1. profile tồn tại và hợp lệ;
2. `deployment_validated=true`, trừ khi lập trình viên chủ động bật override;
3. có adapter cho đúng họ model;
4. backend được compile, khả dụng và được profile khai báo;
5. device và precision được hỗ trợ;
6. đủ artifact và hợp đồng tensor của session khớp metadata.

Không có fallback ngầm sang backend khác và không có đường sinh detection giả.

## Thuật toán suy luận

`DetectorPipeline::detect` dùng mutex để tuần tự hóa một detector instance và chạy các bước:

```text
kiểm tra frame/options/class
        |
        v
adapter.preprocess(image, metadata model)
        |
        v
backend.infer(các tensor FP32 có tên)
        |
        v
adapter.postprocess(tensor output trung lập runtime)
        |
        +--> NMS tùy chọn nếu metadata yêu cầu
        +--> lọc class được chọn
        +--> giới hạn max-detections
        v
DetectionResult + thời gian từng bước + generation của model
```

Tiền xử lý sở hữu dữ liệu ảnh, đổi BGR sang RGB khi metadata yêu cầu, lấy mẫu song tuyến tính và
dùng resize trực tiếp hoặc letterbox căn giữa. Với YOLO26n đã xác thực, scale là
`min(640/source_width, 640/source_height)`, vùng thừa được điền `114`, pixel nhân `1/255`, sau đó
ghi liên tục theo FP32 CHW `[1,3,640,640]`. Biến đổi resize được lưu để ánh xạ box trở lại tọa độ
ảnh gốc.

Output YOLO26 end-to-end đã xác thực là FP32 `[1,N,6]`; mỗi hàng là
`x1,y1,x2,y2,confidence,class_id`. Adapter lọc confidence/class và khôi phục tọa độ; NMS đã nằm
trong graph. Hợp đồng one-to-many riêng là `[1,4+C,N]`: chọn class tốt nhất cho mỗi prediction,
khôi phục tọa độ rồi chạy NMS theo class. Sai rank, batch, channel, số điểm, giá trị không hữu hạn,
class ngoài miền hoặc confidence ngoài miền đều trả lỗi.

## Luồng xử lý Desktop

```text
GUI thread
  AppController -> MainWindow / DetectionViewport
       ^                         |
       | DetectionPacket         | lệnh điều khiển
       |                         v
InferenceWorker std::thread <- một pending frame mới nhất
       ^
       |
CameraWorker QObject trên QThread <- OpenCV VideoCapture + QTimer
```

Camera và inference không chạy trên GUI thread. Nếu camera nhanh hơn inference, frame mới thay
frame đang chờ duy nhất và tăng `droppedFrames`; nhờ vậy độ trễ và bộ nhớ không tăng theo hàng đợi
frame cũ. Yêu cầu nạp model làm tăng generation, xóa frame đang chờ và loại kết quả thuộc
generation trước. Mỗi packet chứa đúng frame sở hữu, detection, nhãn, generation và số frame bị
bỏ, nên box không bị vẽ lên frame khác.

Đổi lựa chọn class chỉ lọc lại kết quả cache để hiển thị, không suy luận lại. Đổi confidence, IoU
hoặc số detection tối đa sẽ gửi lại ảnh tĩnh hiện tại; khi chạy camera, option mới áp dụng cho
frame kế tiếp. Khi đóng app, controller dừng capture, join camera thread, dừng và join inference,
unload detector rồi mới hủy UI.
