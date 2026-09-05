# Desktop Qt ODF

Target `odf_desktop` là client Qt 6 Widgets của public pipeline ODF. Camera Windows được Media
Foundation quét bằng tên thật và symbolic link ổn định. Capture chạy trên `QThread` riêng với
session có acknowledgement; load model và inference chạy trên standard thread được join, với một
vị trí chờ frame mới nhất có kiểm tra generation. Refresh quét lại thiết bị và phục hồi nguồn mà
thông thường không reload model.

Xem [hoạt động Desktop](../../docs/Vn/desktop.md), [build](../../docs/Vn/build.md) và
[hướng dẫn nhanh YOLO26n](../../docs/Vn/yolo26n_quickstart.md).
