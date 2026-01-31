//comment out do disable emissive materials, which are assigned by the iteration count
#define EMISSIVE_MATERIALS

// EMISSIVE ITERATIONS
bool isEmissiveIteration(int k) {
    //set this to the number of list items
    const int EM_COUNT = 1;
    
    //enter specific iterations here
    const int EM_IDS[EM_COUNT] = int[]/* ------> */ (21);
    
    #ifdef EMISSIVE_MATERIALS
    
    for (int i = 0; i < EM_COUNT; ++i) {
        if (k == EM_IDS[i]) {
            return true;
        }
    }
    
    #endif
    
    return false;
}

void GetMaterial(vec3 p, float matId,
                 out vec3 albedo,
                 out float roughness,
                 out float metallic,
                 out vec3 emission)
{
    float iterF = matId * float(FOLD_STEPS - 1);                 
    int   iter  = int(floor(iterF + 0.5));      

    float cycle = matId;                        

    albedo = palette(cycle);

    // Micro variation for the embedded texture
    //float micro = hash31(floor(p * 512.0)) - 0.5;
    //albedo *= 1.0 + 0.08 * micro;

    roughness = clamp(0.15 + 0.8 * abs(sin(TAU * cycle)), 0.05, 0.98);
    metallic  = smoothstep(0.55, 0.95, cycle);

    // Emission: only specific fold iterations glow
    bool emissiveIter = isEmissiveIteration(iter);
    float eMask = emissiveIter ? 1.0 : 0.0;

    // Emission uses the palette color so embedded texture becomes emissive
    emission = albedo * eMask * 2.0;
}
