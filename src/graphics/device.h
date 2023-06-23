#pragma once

#include "instance.h"

class Device {
public:

	void init(Instance &instance);
	void clean();

private:
	VkPhysicalDevice gpu = nullptr;

	void pickPhysicalDevice(Instance &instance);
};