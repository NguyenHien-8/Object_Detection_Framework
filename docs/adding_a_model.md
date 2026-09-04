# Adding a model

1. Implement `IModelAdapter` in the appropriate framework library. Keep runtime and GUI types out.
2. Define one explicit tensor contract and reject every inconsistent shape/type/value.
3. Register the creator below the application layer.
4. Add a versioned `model.json` and label file/reference.
5. Add synthetic decoder tests, invalid-shape tests, preprocessing/coordinate tests, and a real
   reference comparison procedure.
6. Keep `deployment_validated=false` until the real comparison passes.

No change should be needed in common detections, benchmark code, camera capture, or a Qt window.
If it is, first check whether model-specific behavior has leaked out of the adapter.

