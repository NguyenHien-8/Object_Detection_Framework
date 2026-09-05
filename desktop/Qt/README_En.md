# ODF Qt desktop

The `odf_desktop` target is a Qt 6 Widgets client over the public ODF pipeline. Windows cameras
are enumerated by Media Foundation using real names and stable symbolic links. Capture runs on a
dedicated `QThread` with acknowledged sessions; model loading and inference run on a joined
standard thread with a generation-aware capacity-one pending-frame slot. Refresh re-enumerates
devices and recovers source state without normally reloading the model.

See [Desktop behavior](../../docs/En/desktop.md), [build](../../docs/En/build.md), and the
[YOLO26n quickstart](../../docs/En/yolo26n_quickstart.md).
