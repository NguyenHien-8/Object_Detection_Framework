# Desktop Qt ODF

Target `odf_desktop` là client Qt 6 Widgets của public pipeline ODF. Camera chạy trên `QThread`
riêng; load model và inference chạy trên standard thread được join, với một vị trí chờ cho frame
mới nhất.

Xem [hoạt động Desktop](../../docs/Vn/desktop.md), [build](../../docs/Vn/build.md) và
[hướng dẫn nhanh YOLO26n](../../docs/Vn/yolo26n_quickstart.md).
