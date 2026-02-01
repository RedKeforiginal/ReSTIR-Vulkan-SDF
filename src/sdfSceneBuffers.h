#pragma once

#include <nvmath_glsltypes.h>

#include "vma.h"

struct SdfSceneBuffers {
	struct SceneParams {
		nvmath::vec4 sdfScene;
		nvmath::vec4 sdfParams;
	};

	vma::UniqueBuffer sceneParamsBuffer;
	vk::DeviceSize sceneParamsBufferSize = 0;

	[[nodiscard]] static SdfSceneBuffers create(vma::Allocator &allocator) {
		SdfSceneBuffers result;
		result.sceneParamsBufferSize = sizeof(SceneParams);
		result.sceneParamsBuffer = allocator.createTypedBuffer<SceneParams>(
			1, vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU
		);
		auto *params = result.sceneParamsBuffer.mapAs<SceneParams>();
		*params = SceneParams{
			.sdfScene = nvmath::vec4(0.0f, 0.0f, 0.0f, 1.0f),
			.sdfParams = nvmath::vec4(2000.0f, 0.001f, 128.0f, 0.0f)
		};
		result.sceneParamsBuffer.unmap();
		result.sceneParamsBuffer.flush();
		return result;
	}
};
