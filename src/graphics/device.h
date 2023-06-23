#pragma once

#include "instance.h"

#include <limits>

struct QueueFamilies {
	const static uint32_t NOT_AN_ID = std::numeric_limits<uint32_t>::max();
	uint32_t graphicsId = NOT_AN_ID;
	uint32_t presentId = NOT_AN_ID;
	inline bool correct() const { return graphicsId != NOT_AN_ID && presentId != NOT_AN_ID; }
	inline operator const uint32_t*() const { return reinterpret_cast<const uint32_t*>(this); }
};

class Device {
public:
	~Device() { clean(); }

	void init(Instance &instance, VkSurfaceKHR surface);
	void clean();

private:
	VkPhysicalDevice gpu = nullptr;
	VkDevice device = nullptr;

	VkPhysicalDeviceFeatures features;

	QueueFamilies queueFamilies;
	VkQueue graphicsQueue, presentQueue;

	void pickPhysicalDevice(Instance &instance, VkSurfaceKHR surface);
	void createLogicalDevice(VkSurfaceKHR surface);
};