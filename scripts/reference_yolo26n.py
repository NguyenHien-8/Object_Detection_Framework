#!/usr/bin/env python3
"""Run an Ultralytics reference prediction and emit stable JSON detection data."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--model", default="yolo26n.pt")
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--json", type=Path, required=True)
    parser.add_argument("--render", type=Path)
    parser.add_argument("--confidence", type=float, default=0.25)
    args = parser.parse_args()

    import cv2
    import ultralytics
    from ultralytics import YOLO

    result = YOLO(args.model).predict(
        source=str(args.image), imgsz=640, conf=args.confidence, iou=0.45,
        device="cpu", verbose=False
    )[0]
    detections = []
    if result.boxes is not None:
        for box, confidence, class_id in zip(
            result.boxes.xyxy.cpu().tolist(),
            result.boxes.conf.cpu().tolist(),
            result.boxes.cls.cpu().tolist(),
        ):
            index = int(class_id)
            detections.append(
                {
                    "class_id": index,
                    "label": result.names[index],
                    "confidence": confidence,
                    "xyxy": box,
                }
            )
    payload = {
        "ultralytics": ultralytics.__version__,
        "model": args.model,
        "image": str(args.image.resolve()),
        "confidence_threshold": args.confidence,
        "detections": detections,
    }
    args.json.parent.mkdir(parents=True, exist_ok=True)
    args.json.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    if args.render:
        args.render.parent.mkdir(parents=True, exist_ok=True)
        if not cv2.imwrite(str(args.render), result.plot()):
            raise SystemExit(f"Could not write reference rendering: {args.render}")
    print(f"detections={len(detections)} json={args.json.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

