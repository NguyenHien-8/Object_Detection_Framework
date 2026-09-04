#!/usr/bin/env python3
"""Print a reproducible ONNX graph contract and artifact hash."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def value_description(value, onnx_module) -> str:
    tensor = value.type.tensor_type
    dimensions = []
    for dimension in tensor.shape.dim:
        if dimension.HasField("dim_value"):
            dimensions.append(str(dimension.dim_value))
        elif dimension.HasField("dim_param"):
            dimensions.append(dimension.dim_param)
        else:
            dimensions.append("?")
    dtype = onnx_module.TensorProto.DataType.Name(tensor.elem_type)
    return f"name={value.name} shape=[{','.join(dimensions)}] dtype={dtype}"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("model", type=Path)
    args = parser.parse_args()
    path = args.model.resolve()
    if not path.is_file():
        raise SystemExit(f"ONNX artifact not found: {path}")
    try:
        import onnx
    except ImportError as error:
        raise SystemExit("Missing development dependency. Install with: pip install onnx") from error

    model = onnx.load(str(path), load_external_data=False)
    onnx.checker.check_model(model)
    initializer_names = {item.name for item in model.graph.initializer}
    print(f"onnx={onnx.__version__}")
    print(f"artifact={path}")
    print(f"sha256={sha256(path)}")
    print("opset=" + ",".join(f"{item.domain or 'ai.onnx'}:{item.version}" for item in model.opset_import))
    for item in model.graph.input:
        if item.name not in initializer_names:
            print("input " + value_description(item, onnx))
    for item in model.graph.output:
        print("output " + value_description(item, onnx))
    for item in model.metadata_props:
        print(f"metadata {item.key}={item.value}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
