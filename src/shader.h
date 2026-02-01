#pragma once

#include <cstdlib>

#include "vulkanConfig.h"

#include "misc.h"

class Shader {
public:
	[[nodiscard]] vk::ShaderModule get() const {
		return _module.get();
	}
	[[nodiscard]] const vk::PipelineShaderStageCreateInfo &getStageInfo() const {
		return _shaderInfo;
	}

	[[nodiscard]] bool empty() const {
		return !_module;
	}
	[[nodiscard]] explicit operator bool() const {
		return static_cast<bool>(_module);
	}

	[[nodiscard]] inline static vk::UniqueShaderModule loadModule(
		vk::Device dev, const std::filesystem::path &path
	) {
		std::vector<char> binary = readFile(path);
		if (binary.empty()) {
			std::cerr << "Shader binary is empty: " << path << "\n";
			std::abort();
		}
		if (binary.size() % sizeof(uint32_t) != 0) {
			std::cerr << "Shader binary size is not 4-byte aligned: " << path << "\n";
			std::abort();
		}

		vk::ShaderModuleCreateInfo shaderInfo;
		shaderInfo
			.setCodeSize(binary.size())
			.setPCode(reinterpret_cast<const uint32_t*>(binary.data()));
		return dev.createShaderModuleUnique(shaderInfo);
	}
	[[nodiscard]] inline static Shader load(
		vk::Device dev, const std::filesystem::path &path, const char *entry, vk::ShaderStageFlagBits stage
	) {
		Shader result;
		result._module = loadModule(dev, path);
		result._shaderInfo
			.setModule(result._module.get())
			.setPName(entry)
			.setStage(stage);
		return result;
	}
private:
	vk::UniqueShaderModule _module;
	vk::PipelineShaderStageCreateInfo _shaderInfo;
};
