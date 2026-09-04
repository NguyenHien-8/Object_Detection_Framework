#!/usr/bin/env python3
"""Export official Ultralytics YOLO26n weights to the ODF ONNX artifact path."""

from __future__ import annotations

import argparse
import hashlib
import shutil
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--weights", default="yolo26n.pt", help="Official weights path or name")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("models/yolo26/yolo26n/model.onnx"),
        help="Destination ONNX path",
    )
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--opset", type=int, default=None)
    args = parser.parse_args()

    try:
        import ultralytics
        from ultralytics import YOLO
    except ImportError as error:
        raise SystemExit(
            "Missing development dependency. Install with: pip install ultralytics onnx"
        ) from error

    model = YOLO(args.weights)
    export_options = {
        "format": "onnx",
        "imgsz": args.imgsz,
        "dynamic": False,
        "simplify": False,
        "nms": False,
        "device": "cpu",
        "batch": 1,
    }
    if args.opset is not None:
        export_options["opset"] = args.opset
    exported = Path(model.export(**export_options)).resolve()
    destination = args.output.resolve()
    destination.parent.mkdir(parents=True, exist_ok=True)
    if exported != destination:
        shutil.copy2(exported, destination)

    print(f"ultralytics={ultralytics.__version__}")
    print(f"weights={args.weights}")
    print(f"artifact={destination}")
    print(f"sha256={sha256(destination)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
