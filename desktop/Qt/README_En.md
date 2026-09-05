# ODF Qt desktop

The `odf_desktop` target is a Qt 6 Widgets client over the public ODF pipeline. Camera capture
runs on a dedicated `QThread`; model loading and inference run on a joined standard thread with a
capacity-one pending-frame slot.

See [Desktop behavior](../../docs/En/desktop.md), [build](../../docs/En/build.md), and the
[YOLO26n quickstart](../../docs/En/yolo26n_quickstart.md).
