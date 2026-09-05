# Dependency và đóng gói trên Windows

Dùng thống nhất x64 Release cho ứng dụng và mọi native dependency. Tổ hợp cục bộ đã kiểm tra là
Visual Studio 2022/MSVC 19.44, CMake 4.4.2, Qt 6.11.2 MSVC2022 x64, OpenCV 4.14.0 và ONNX Runtime
C++ 1.26.0. Phiên bản CMake tối thiểu của project vẫn là 3.24.

Camera được quét bằng Windows Media Foundation từ Windows SDK (`mf`, `mfplat`, `mfuuid`) và được
capture bằng backend OpenCV `CAP_MSMF` tương ứng. Không cần Qt Multimedia.

## Các file mốc của SDK

```text
C:\Qt\6.11.2\msvc2022_64\lib\cmake\Qt6\Qt6Config.cmake
C:\SDK\opencv-4.14.0\opencv\build\x64\vc16\lib\OpenCVConfig.cmake
C:\SDK\onnxruntime-1.26.0\include\onnxruntime_cxx_api.h
C:\SDK\onnxruntime-1.26.0\lib\onnxruntime.lib
C:\SDK\onnxruntime-1.26.0\lib\onnxruntime.dll
```

Một số gói ONNX Runtime để DLL dưới `bin`; project kiểm tra các vị trí được hỗ trợ. Không trộn
Debug DLL (thường có đuôi `d.dll`), thư viện x86, Qt MinGW hoặc ABI MSVC khác với executable x64
Release.

## Stage runtime trong build tree

Build `odf_desktop` sẽ chạy `windeployqt --no-translations` nếu có, copy ONNX Runtime DLL, OpenCV
world Release DLL và toàn bộ cây model cục bộ. Nhờ đó thư mục `Release` có thể chạy trên máy phát
triển.

## Thư mục phân phối

Tạo `dist/` riêng bằng:

```powershell
.\scripts\deploy_windows.ps1 `
  -BuildDirectory .\build-desktop `
  -Configuration Release `
  -QtBin "C:\Qt\6.11.2\msvc2022_64\bin" `
  -OpenCvBin "C:\SDK\opencv-4.14.0\opencv\build\x64\vc16\bin" `
  -OnnxRuntimeRoot "C:\SDK\onnxruntime-1.26.0" `
  -ModelRoot .\models
```

Trước khi phân phối, cần có ít nhất `odf_desktop.exe`, `onnxruntime.dll`, OpenCV Release
`opencv_world*.dll`, `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `platforms/qwindows.dll`, JSON
registry/nhãn và `models/yolo26/yolo26n/model.onnx`. Đồng thời xem lại giấy phép của model và mọi
thành phần bên thứ ba.

Script deploy tạo hoặc cập nhật đúng thư mục đích được truyền; nó không chứng minh gói chạy trên
máy sạch. Trước khi phát hành, hãy thử thư mục đã đóng gói trên máy không có đường dẫn development
trong `PATH`.

`windeployqt` có thể cảnh báo chưa đặt `VCINSTALLDIR` khi chạy từ PowerShell thông thường. Nếu
build thành công và đủ DLL/plugin như checklist trên thì cảnh báo này không gây lỗi. Hãy dùng
Visual Studio Developer PowerShell khi thiếu file deploy hoặc khi tạo gói phát hành.
