#pragma once

#include <vulkan/vulkan.h>

struct Extensions {
	uint32_t count;
	const char** exts;
};

class Instance {
public:
	~Instance() { clean(); }

	void init(const char* name, const Extensions &requiredExtensions);
	void clean();

	inline operator VkInstance() { return instance; }

	#ifndef NDEBUG
	const static char* validation_layer;
	#endif

private:
	VkInstance instance = nullptr;

	#ifndef NDEBUG
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	#endif
};