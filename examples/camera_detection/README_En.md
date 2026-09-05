# Camera detection CLI

`odf_camera_detection` captures on a dedicated thread and passes frames through the core
capacity-one `LatestFrameBuffer`; stale pending frames are replaced when inference is slower than
capture.

```powershell
.\build-desktop\examples\camera_detection\Release\odf_camera_detection.exe `
  --models .\models --model yolo26n --backend onnxruntime --camera 0
```

Press Esc or Q to stop. Camera hardware is deliberately excluded from ordinary tests.
