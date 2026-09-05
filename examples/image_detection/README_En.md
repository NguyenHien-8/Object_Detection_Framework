# Image detection CLI

`odf_image_detection` uses `RuntimeCatalog`, `DetectorFactory`, the selected adapter, and the
selected real backend. It does not contain a second YOLO decoder.

```powershell
.\build-desktop\examples\image_detection\Release\odf_image_detection.exe `
  --models .\models --model yolo26n --backend onnxruntime `
  --image "C:\path\to\input.jpg" --output "C:\path\to\result.jpg"
```

See the [YOLO26n quickstart](../../docs/En/yolo26n_quickstart.md).
