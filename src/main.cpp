#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>
#undef TINYGLTF_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION

#ifdef _MSC_VER
#	define STBI_MSC_SECURE_CRT
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#undef STB_IMAGE_WRITE_IMPLEMENTATION

#include <gflags/gflags.h>

#include "app.h"

DEFINE_string(scene, "", "Path to the scene file.");
DEFINE_bool(ignore_point_lights, false, "Ignore point lights in the scene.");
DEFINE_bool(enable_validation, false, "Enable Vulkan validation layers and debug utils.");
DEFINE_bool(validate_assets, false, "Validate shader/scene assets before startup.");

int main(int argc, char **argv) {
	gflags::ParseCommandLineFlags(&argc, &argv, true);
	App app(FLAGS_scene, FLAGS_ignore_point_lights, FLAGS_enable_validation, FLAGS_validate_assets);
	app.mainLoop();
	return 0;
}
