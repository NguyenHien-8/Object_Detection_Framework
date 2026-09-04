# ODF Qt desktop

The desktop target is `odf_desktop`. It is a thin Qt 6 Widgets client over the public ODF
pipeline and runtime catalog. Camera capture runs on a dedicated `QThread`; model loading and
inference run on a separate standard worker thread with a one-frame latest-frame queue.

See `docs/desktop.md` and `docs/yolo26n_quickstart.md` for build and run instructions.
