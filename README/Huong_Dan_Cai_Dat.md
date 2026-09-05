# Hướng Dẫn Cài Đặt

## 1. Cài cl.exe — MSVC 2022

### Bước 1.1 — Tải Build Tools 2022
```Powershell
https://aka.ms/vs/17/release/vs_BuildTools.exe?utm_source
```

### Bước 1.2 — Chạy installer
- Chạy: vs_BuildTools.exe
- Trong tab Workloads, tick: Desktop development with C++
- Ở panel bên phải, đảm bảo có ít nhất:
+ MSVC v143 - VS 2022 C++ x64/x86 build tools
+ Windows 11 SDK
+ C++ CMake tools for Windows

### Bước 1.3 — Installation location
- Tốt nhất giữ đường dẫn mặc định:
```
C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools
```
- Nhấn: Install

### Bước 1.4 — Kiểm tra đã cài đặt cl và MSVC hay chưa
- Vào Search trên Window và gõ: Developer PowerShell for VS 2022
- Sau đó gõ cl
+ Sẽ thấy tương tự: Microsoft (R) C/C++ Optimizing Compiler Version 19.xx.xxxxx for x64

## 2. Cài Qt 6

### Bước 2.1 — Tải Qt Online Installer
```Powershell
https://download.qt.io/official_releases/online_installers/qt-online-installer-windows-x64-online.exe?utm_source
```
Qt cung cấp chính thức installer Windows x64 này.

### Bước 2.2 — Chạy installer
- Chạy: qt-online-installer-windows-x64-online.exe
- Ở đường dẫn cài đặt chọn: C:\Qt

### Bước 2.3 — Chọn component
- Trong Select Components, chọn:
```
Qt
└── Qt 6.11.2
    └── MSVC 2022 64-bit
```
- Không chọn:
```
MinGW
MSVC ARM64
WebAssembly
Android
Sources
Debug Information
```