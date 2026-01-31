#ifndef FOLD_STEPS
#define FOLD_STEPS 24
#endif

#ifndef FOLD_FLOOR
#define FOLD_FLOOR 1.0
#endif

#include "SDF.glsl"

float getSdfDistance(vec3 worldPos) {
	float scale = uniforms.sdfScene.w;
	vec3 sdfPos = worldPos * scale + uniforms.sdfScene.xyz;
	return GetDist(sdfPos) / max(scale, 0.0001);
}

bool testVisibility(vec3 p1, vec3 p2) {
	vec3 dir = p2 - p1;
	float totalDistance = length(dir);
	if (totalDistance <= 0.0f) {
		return false;
	}
	vec3 rayDir = dir / totalDistance;

	float maxDistance = min(uniforms.sdfParams.x, totalDistance);
	float epsilon = uniforms.sdfParams.y;
	int maxSteps = int(uniforms.sdfParams.z);

	float t = max(epsilon, 0.001f);
	for (int i = 0; i < maxSteps; ++i) {
		vec3 samplePos = p1 + rayDir * t;
		float dist = getSdfDistance(samplePos);
		if (dist < epsilon) {
			return true;
		}
		t += dist;
		if (t >= maxDistance) {
			break;
		}
	}

	return false;
}
