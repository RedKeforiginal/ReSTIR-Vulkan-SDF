#version 450

#define FOLD_STEPS 24
#define FOLD_FLOOR 1.0

#include "include/SDF.glsl"
#include "include/SDF-Material.glsl"

layout (set = 0, binding = 0) uniform Uniforms {
	mat4 projectionMatrix;
	mat4 viewMatrix;
	mat4 inverseProjectionMatrix;
	mat4 inverseViewMatrix;
	vec4 cameraPosition;
	vec4 sdfParams;
	vec4 sdfScene;
} uniforms;

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outAlbedo;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outMaterialProperties;
layout (location = 3) out vec3 outWorldPosition;

float getSdfDistance(vec3 worldPos) {
	float scale = uniforms.sdfScene.w;
	vec3 sdfPos = worldPos * scale + uniforms.sdfScene.xyz;
	return GetDist(sdfPos) / max(scale, 0.0001);
}

vec3 estimateNormal(vec3 worldPos, float epsilon) {
	vec2 h = vec2(epsilon, 0.0);
	float dx = getSdfDistance(worldPos + h.xyy) - getSdfDistance(worldPos - h.xyy);
	float dy = getSdfDistance(worldPos + h.yxy) - getSdfDistance(worldPos - h.yxy);
	float dz = getSdfDistance(worldPos + h.yyx) - getSdfDistance(worldPos - h.yyx);
	return normalize(vec3(dx, dy, dz));
}

void main() {
	float maxDistance = uniforms.sdfParams.x;
	float epsilon = uniforms.sdfParams.y;
	int maxSteps = int(uniforms.sdfParams.z);

	vec2 ndc = inUv * 2.0f - 1.0f;
	vec4 clip = vec4(ndc, 1.0f, 1.0f);
	vec4 view = uniforms.inverseProjectionMatrix * clip;
	view /= view.w;
	vec3 rayDir = normalize((uniforms.inverseViewMatrix * vec4(view.xyz, 0.0f)).xyz);
	vec3 rayOrigin = uniforms.cameraPosition.xyz;

	float t = 0.0f;
	bool hit = false;
	float matId = 0.0f;
	vec3 worldPos = rayOrigin;

	for (int i = 0; i < maxSteps; ++i) {
		worldPos = rayOrigin + rayDir * t;
		float scale = uniforms.sdfScene.w;
		vec3 sdfPos = worldPos * scale + uniforms.sdfScene.xyz;
		vec2 distMat = GetDistMat(sdfPos);
		float dist = distMat.x / max(scale, 0.0001);
		if (dist < epsilon) {
			hit = true;
			matId = distMat.y;
			break;
		}
		t += dist;
		if (t > maxDistance) {
			break;
		}
	}

	if (!hit) {
		outAlbedo = vec4(0.0f);
		outNormal = vec3(0.0f);
		outMaterialProperties = vec2(0.0f);
		outWorldPosition = vec3(0.0f);
		gl_FragDepth = 1.0f;
		return;
	}

	vec3 normal = estimateNormal(worldPos, epsilon * 2.0f);
	vec3 albedo;
	float roughness;
	float metallic;
	vec3 emission;
	vec3 sdfPos = worldPos * uniforms.sdfScene.w + uniforms.sdfScene.xyz;
	GetMaterial(sdfPos, matId, albedo, roughness, metallic, emission);

	float emissiveFlag = length(emission) > 0.0f ? 1.0f : 0.0f;
	outAlbedo = vec4(emissiveFlag > 0.5f ? emission : albedo, emissiveFlag);
	outNormal = normal;
	outMaterialProperties = vec2(roughness, metallic);
	outWorldPosition = worldPos;

	vec4 clipPos = uniforms.projectionMatrix * uniforms.viewMatrix * vec4(worldPos, 1.0f);
	gl_FragDepth = clipPos.z / clipPos.w;
}
