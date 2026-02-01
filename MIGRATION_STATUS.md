# SDF Migration Status

- NOTE: This list is incomplete

## Completed
- SDF ray-marched G-Buffer is active via the compute GBuffer pass (`gBuffer.comp` uses `GetDistMat` + `GetMaterial`) and is wired into ReSTIR/lighting through existing G-Buffer bindings.
- SDF visibility testing is implemented in `visibilityTest.glsl`, replacing trace-ray/BVH logic in the reuse paths.
- Emissive SDF sampling is implemented as a compute pass (`emissiveSample.comp`) feeding the ReSTIR candidate buffer.
- Updated the emissive sample compute pass to synchronize exclusively with compute stages, removing a lingering ray-tracing pipeline barrier.
- Ensured all G-Buffer attachments (including material properties) are transitioned for shader reads so SDF compute passes have consistent inputs on startup.
- Replaced the raster G-Buffer pass with a compute-driven SDF G-Buffer (`gBuffer.comp`) and storage-image descriptors, so the SDF path no longer depends on GLTF-era graphics pipelines.
- Switched G-Buffer formats to storage-compatible floating-point formats and added storage image usage/layout transitions to support compute writes.

## Remaining
- Remove or quarantine unused GLTF/RT-era shader assets that are still checked in but no longer built or referenced (e.g., `hwVisibilityTest.*`, `restirOmniHardware.rgen`, `unbiasedReuseHardware.rgen`).
- Remove or refactor demo/simple pipeline assets that are no longer part of the SDF runtime (e.g., `demoPass.h`, `simple.vert/frag`, `vertex.h`) once confirmed unused.
- Update `README.md` to describe the SDF scene pipeline instead of GLTF loading, including the removal of `-scene`/`-ignore_point_lights` references and tinygltf mentions.
- Rename/clean up remaining GLTF-era terminology in code (e.g., `VisibilityTestMethod::software` naming) to reflect SDF shadow marching.
- Validate/tune SDF parameters and expose runtime controls (max steps, epsilon, max distance, scene transform) via UI and/or config.
- The CMake target still compiles demo/simple shaders that are unused by the SDF rendering path.
- More work to be determined.