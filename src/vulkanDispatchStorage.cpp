#include "vulkanConfig.h"

#if defined(VULKAN_HPP_DISPATCH_LOADER_DYNAMIC) && !defined(VULKAN_HPP_NO_DEFAULT_DISPATCHER)
namespace vk::detail {
	DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}
#endif
