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
- Implement ReSTIR candidate generation directly from emissive SDF materials by sampling `GetDistMat`/`GetMaterial` in `SDF.glsl` and `SDF-Material.glsl`, then writing emissive surface samples into the candidate buffer (compute path should own emissive discovery rather than relying on legacy light buffers).
- Add a dedicated emissive sampling strategy: define a bounded sampling volume, generate stochastic ray-marched hits against the SDF, and cache emissive hits in a GPU buffer refreshed every frame or every N frames to keep candidate distributions stable.
- Integrate emissive material emission strength and area heuristics into candidate weights (convert emission color/intensity into the reservoir `w` term and add PDF estimates for emissive surface sampling).
- Update ReSTIR shaders to treat emissive SDF materials as the sole light source, removing any remaining dependency on point/triangle light buffers or alias tables used for GLTF scenes.
- Lighting still resolves via a graphics pipeline (full-screen quad) using raster state setup in `pass.h` and `lightingPass.h` (`quad.vert` + `lighting.frag`). If the goal is compute-only rendering, convert this resolve to a compute pass or an explicit compute-to-swapchain blit.
- ImGui rendering still relies on a Vulkan graphics render pass. Decide whether UI remains raster-based as an intentional exception or replace it with a compute-driven overlay path.
- Legacy GLTF/tinygltf third-party sources are still present in `thirdparty/` (and appear in historical build logs). If they are no longer referenced by the build, remove them to avoid accidental GLTF reintroduction.
