# Real-model integration validation

For each model/profile/backend tuple:

1. Record the upstream repository commit/release, checkpoint hash, exported artifact hash, export
   command, runtime version, device, and precision.
2. Use one fixed lossless input image and identical resize, color, normalization, confidence,
   IoU, class filter, and postprocess mode in C++ and the upstream reference.
3. Capture runtime-reported tensor names, types, and shapes before decoding.
4. Compare detection count and class IDs exactly. Compare confidence and original-image box
   coordinates using documented numeric tolerances appropriate to the precision.
5. Add the image/hash and expected detection fixture to an optional integration test. Do not make
   ordinary core tests require model downloads or a GPU.
6. Repeat across all requested profiles; validating one profile does not validate sibling exports.

A model is not “supported” merely because the runtime loaded it. The current repository has not
performed these real-model steps because no artifacts or runtime SDKs were present.

