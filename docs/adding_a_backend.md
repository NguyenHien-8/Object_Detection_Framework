# Adding a backend

1. Implement `IInferenceBackend` in `backends/<name>` and hide vendor headers in its source/PImpl.
2. Add an opt-in CMake flag and use `find_package` or a documented root hint. Never download a
   large SDK during ordinary configuration.
3. Report real devices, precision, formats, and availability via `BackendInfo`.
4. Validate artifact files and actual session input/output metadata during load.
5. Register a creator with `DetectorFactory` in the application or a small integration library.
6. Add tests with the real SDK where available and document the exact SDK version.

Do not decode model boxes in a backend, include vendor types in `odf_core`, silently fall back, or
report success before the runtime executed.

