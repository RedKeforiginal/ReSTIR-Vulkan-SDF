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
- Removed demo/simple pipeline assets and their build hooks (`demoPass.h`, `vertex.h`, `simple.vert/frag`, shader asset validation), keeping only the SDF runtime passes.
- Deleted legacy RT-era shader stages (`hwVisibilityTest.*`, `restirOmniHardware.rgen`, `unbiasedReuseHardware.rgen`) and raster G-Buffer stages (`gBuffer.vert/.frag`) that are no longer part of the SDF pipeline.
- Updated the README to document the SDF scene pipeline and note the removal of legacy GLTF flags.
- Renamed visibility test terminology in code/UI to reference SDF shadow marching instead of legacy software RT naming.

## Remaining
- Remove or quarantine unused GLTF/tinygltf third-party sources if they are no longer part of the build graph.
- `thirdparty/gltf/` (plus `thirdparty/tinygltf/` headers) are still present on disk, but no sources reference them or add them to the build graph.
- More work to be determined.
