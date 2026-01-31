# AGENTS.md

## Project State (Current)
- The renderer currently builds G-Buffers from rasterized GLTF geometry via `GBufferPass`, outputting albedo (with emissive flag in alpha), normal, material properties, world position, and depth targets for downstream passes.
- ReSTIR passes (`RestirPass`, `UnbiasedReusePass`, and `SpatialReusePass`) rely on G-Buffer textures plus light buffers (point/triangle lights + alias table) sourced from `SceneBuffers`.
- Visibility testing is performed either with Vulkan hardware ray tracing (RT pipelines + SBT + TLAS) or a software BVH (AABB tree + triangle buffers), wired through `visibilityTest.glsl` and used by both ReSTIR and unbiased reuse.
- Lighting resolve (`lighting.frag`) reads reservoirs and uses stored G-Buffer data (with emissive materials indicated by albedo alpha) to compute final shading.

## High-Level Design: Convert to SDF Ray Marching + SDF Materials

### Goals
- Render the SDF scene defined in `src/shaders/include/SDF.glsl` and use material assignment from `src/shaders/include/SDF-Material.glsl`.
- Replace all ray tracing (hardware + software BVH) with SDF ray marching while keeping ReSTIR logic intact.
- Use emissive SDF materials as the sole emission sources for ReSTIR sampling.

### 1) Replace Rasterized G-Buffer with SDF Ray-Marched G-Buffer
- Introduce a new SDF-based G-Buffer pass (fragment or compute) that ray marches from the camera through the SDF.
- Outputs must match current G-Buffer layout: albedo (with emissive flag in alpha), normal, material properties, world position, and depth.
- Surface properties are derived from `GetDistMat` (material id) + `GetMaterial` (albedo/roughness/metallic/emission).
- Resulting G-Buffer stays compatible with existing ReSTIR and lighting passes.

### 2) Replace Visibility Tests with SDF Shadow Ray Marching
- Implement `testVisibility` as a shadow-ray march against the SDF, replacing `traceRayEXT` and software BVH traversal.
- Shadow rays march from shaded point toward candidate light positions, early-outing on hit or reaching the target.
- Remove RT descriptor bindings and AABB tree buffers from shaders once SDF visibility is in place.

### 3) Replace Light Buffers with SDF Emissive Sampling
- Emissive SDF materials become the light source set.
- Options for candidate generation:
  - **Preferred (runtime cache, not offline precompute):** Stochastically *populate* a GPU buffer of emissive surface samples via a compute pass (e.g., random ray marching within a bounded region, refreshed every frame or every N frames). This is an approximate, runtime cache—not a deterministic precompute—since the fractal SDF has no explicit surface list or known area distribution.
  - **Alternative:** Per-pixel stochastic emissive ray marching (higher variance, no shared cache).
- ReSTIR reservoirs remain unchanged; only the candidate source and visibility test change.

### 4) Remove Hardware/Software RT Pipelines
- Remove RT pipeline creation, SBT setup, and RT descriptor sets from `RestirPass` and `UnbiasedReusePass`.
- Keep compute pipelines only, using SDF visibility tests.
- Remove RT-specific initialization in `App` (TLAS buffers, RT descriptor pools, visibility method toggles).

### 5) Keep Lighting & ReSTIR Logic Intact
- Preserve reservoir math, temporal/spatial reuse, and final lighting evaluation.
- Ensure emissive materials still show through when the G-Buffer marks emissive in albedo alpha, or integrate emission fully via ReSTIR samples.

### 6) Add SDF Tuning Controls
- Expose SDF parameters (max steps, epsilon, max distance, scene scale/offset) via uniforms and UI.
- Keep constants (`FOLD_STEPS`, `FOLD_FLOOR`) aligned with the new SDF scene definitions.

### Suggested Migration Order
1. Implement SDF G-Buffer pass and validate albedo/normal/world-position outputs.
2. Replace `testVisibility` with SDF shadow marching.
3. Add emissive sampling buffer and integrate into ReSTIR candidate selection.
4. Remove RT/BVH pipeline and buffers.
5. Tune parameters for quality/performance.
