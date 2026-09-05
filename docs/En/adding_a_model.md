# Adding a model

1. Implement `IModelAdapter` in the matching framework library. Keep runtime and UI types out.
2. Define one explicit tensor contract and reject inconsistent rank, shape, type, values, and
   class ranges.
3. Register the adapter creator through the framework registration function used by
   `RuntimeCatalog`.
4. Add a versioned `model.json` and a label file/reference under `models/<family>/<profile>/`.
5. Add synthetic decoder tests, invalid-contract tests, preprocessing/coordinate tests, and the
   real-reference procedure.
6. Test through `DetectorFactory` so the adapter/backend composition is exercised.
7. Keep `deployment_validated=false` until comparison against the pinned upstream implementation
   passes for the exact artifact hash.

Model-specific decoding belongs only in the adapter. Common detections, camera code, benchmark
code, backends, and Qt widgets should not need model-family conditionals.

If the artifact output differs from an existing family contract, add a new explicit postprocess
mode and tests; do not reinterpret it based on filename or dimensions guessed at runtime.
