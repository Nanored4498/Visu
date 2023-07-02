#pragma once

#include "instance.h"
#include "window.h"

#include <vector>

namespace gfx {

struct QueueFamilies {
	const static uint32_t NOT_AN_ID;
	uint32_t graphicsId = NOT_AN_ID;
	uint32_t presentId = NOT_AN_ID;
	inline bool correct() const { return graphicsId != NOT_AN_ID && presentId != NOT_AN_ID; }
	inline operator const uint32_t*() const { return reinterpret_cast<const uint32_t*>(this); }
};

class CommandBuffer;

class Device {
public:
	~Device() { clean(); }

	void init(Instance &instance, const Window &window);
	void clean();

	inline operator VkDevice() const { return device; }

	CommandBuffer createCommandBuffer(bool primary=true) const;
	void allocCommandBuffers(CommandBuffer *cmdBufs, uint32_t size, bool primary=true) const;
	inline void freeCommandBuffer(CommandBuffer& cmdBuf) const {
		if(device) vkFreeCommandBuffers(device, commandPool, 1u, reinterpret_cast<VkCommandBuffer*>(&cmdBuf));
	}
	inline void freeCommandBuffers(CommandBuffer *cmdBufs, uint32_t size) const {
		if(device) vkFreeCommandBuffers(device, commandPool, size, reinterpret_cast<VkCommandBuffer*>(cmdBufs));
	}

	inline void waitIdle() const { vkDeviceWaitIdle(device); }

	inline VkPhysicalDevice getGPU() const { return gpu; }
	inline VkQueue getGraphicsQueue() const { return graphicsQueue; }
	inline VkQueue getPresentQueue() const { return presentQueue; }
	inline const QueueFamilies& getQueueFamilies() const { return queueFamilies; }

	inline std::vector<VkSurfaceFormatKHR> getSurfaceFormats(const Window &window) const {
		return vkGetList(vkGetPhysicalDeviceSurfaceFormatsKHR, gpu, window.getSurface());
	}
	inline std::vector<VkPresentModeKHR> getPresentModes(const Window &window) const {
		return vkGetList(vkGetPhysicalDeviceSurfacePresentModesKHR, gpu, window.getSurface());
	}
	inline VkSurfaceCapabilitiesKHR getCapabilities(const Window &window) const {
		VkSurfaceCapabilitiesKHR capabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, window.getSurface(), &capabilities);
		return capabilities;
	}

private:
	VkPhysicalDevice gpu = nullptr;
	VkDevice device = nullptr;

	VkPhysicalDeviceFeatures features;

	QueueFamilies queueFamilies;
	VkQueue graphicsQueue, presentQueue;

	VkCommandPool commandPool;
};

}