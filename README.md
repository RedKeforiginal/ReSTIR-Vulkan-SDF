This is modified version of the ReSTIR - Vulkan project by:

 - **Xuanyi Zhou** [[GitHub](https://github.com/lukedan)] [[LinkedIn](https://www.linkedin.com/in/xuanyi-zhou-661365192/)]
 - **Xuecheng Sun** [[GitHub](https://github.com/hehehaha12139)] [[LinkedIn](https://www.linkedin.com/in/hehehaha12138/)]
 - **Jiarui Yan** [[GitHub](https://github.com/WaikeiChan)] [[LinkedIn](https://www.linkedin.com/in/jiarui-yan-a06bb5197/)]


## SDF renderer (current)

This fork focuses on an SDF (signed distance field) scene pipeline instead of GLTF asset loading. The renderer:

- Ray marches the SDF scene defined in `src/shaders/include/SDF.glsl` and material assignments in `src/shaders/include/SDF-Material.glsl`.
- Builds G-Buffers via a compute pass (`gBuffer.comp`) and feeds them into ReSTIR reuse/lighting passes.
- Samples emissive SDF materials as the only light sources for ReSTIR candidates.

GLTF scene loading and legacy `-scene` or `-ignore_point_lights` options are no longer part of the runtime. The focus is on tuning SDF parameters and reuse settings through the in-app UI and shader constants.
