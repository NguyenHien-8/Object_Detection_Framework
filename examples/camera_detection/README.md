# Camera detection CLI

`odf_camera_detection` captures on a dedicated thread and uses the core capacity-one
`LatestFrameBuffer`, so stale frames are replaced when inference is slower than capture.

```powershell
odf_camera_detection.exe --models models --model yolo26n --backend onnxruntime --camera 0
```

Press Esc or Q to stop. Camera hardware is deliberately excluded from ordinary unit tests.

