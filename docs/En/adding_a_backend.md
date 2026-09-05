# Adding a backend

1. Implement `IInferenceBackend` under `backends/<name>` and hide vendor headers in source/PImpl.
2. Add an opt-in CMake option and use `find_package` or a documented SDK-root cache variable.
   Ordinary configuration must not download a large SDK.
3. Report real devices, precisions, formats, availability, and acceleration capabilities through
   `BackendInfo`.
4. Validate artifact files plus actual session input/output names, types, counts, and supported
   shapes during load.
5. Accept ODF's named runtime-neutral tensors and return real runtime outputs without decoding
   model boxes in the backend.
6. Register the backend with `RuntimeCatalog` only when its target is compiled.
7. Stage required runtime libraries for applications, add SDK-backed tests, and document exact
   validated versions and deployment requirements.

Never silently fall back to another backend, leak vendor types into `odf_core`, return dummy
detections, or advertise success before executing the real runtime.
