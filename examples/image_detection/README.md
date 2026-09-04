# Image detection CLI

`odf_image_detection` reuses `RuntimeCatalog`, `DetectorFactory`, the selected model adapter, and
the selected inference backend. It does not contain a second YOLO decoder.

```powershell
odf_image_detection.exe --models models --model yolo26n --backend onnxruntime `
  --image input.jpg --output result.jpg
```

