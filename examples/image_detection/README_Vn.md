# CLI nhận diện ảnh

`odf_image_detection` dùng `RuntimeCatalog`, `DetectorFactory`, adapter được chọn và backend thật
được chọn. Nó không chứa decoder YOLO thứ hai.

```powershell
.\build-desktop\examples\image_detection\Release\odf_image_detection.exe `
  --models .\models --model yolo26n --backend onnxruntime `
  --image "C:\path\to\input.jpg" --output "C:\path\to\result.jpg"
```

Xem [hướng dẫn nhanh YOLO26n](../../docs/Vn/yolo26n_quickstart.md).
