# Torch adapter examples

NanoDet and YOLO26 composition is covered by mock-backend and synthetic tensor tests. Real image
execution is provided by the shared image CLI only when a compiled runtime and a matching local
artifact are available. Production code never substitutes synthetic detections. See
[models and artifacts](../../docs/En/models.md).
