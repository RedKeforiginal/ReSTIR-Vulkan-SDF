#pragma once

#include <array>

#include "vulkanConfig.h"

#include <nvmath_glsltypes.h>

struct Vertex {
	nvmath::vec4 position;
	nvmath::vec4 normal;
	nvmath::vec4 tangent;
	nvmath::vec4 color;
	nvmath::vec2 uv;
};
