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

private:
	VkInstance instance = nullptr;
	#ifndef NDEBUG
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	#endif
};