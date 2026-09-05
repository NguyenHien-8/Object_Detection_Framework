# CLI nhận diện camera

`odf_camera_detection` capture trên thread riêng và truyền frame qua `LatestFrameBuffer` dung
lượng một của core; frame đang chờ cũ bị thay khi inference chậm hơn camera.

```powershell
.\build-desktop\examples\camera_detection\Release\odf_camera_detection.exe `
  --models .\models --model yolo26n --backend onnxruntime --camera 0
```

Nhấn Esc hoặc Q để dừng. Test thông thường chủ động không phụ thuộc phần cứng camera.
