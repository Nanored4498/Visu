// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

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

	void init(const Window &window, VkPhysicalDevice gpu);
	void clean();

	inline operator VkDevice() const { return device; }

	static std::vector<VkPhysicalDevice> getAvailableDevices(const Instance &instance, const Window &window);

	CommandBuffer createCommandBuffer(bool primary=true) const;
	void allocCommandBuffers(CommandBuffer *cmdBufs, uint32_t size, bool primary=true) const;
	inline void freeCommandBuffers(CommandBuffer *cmdBufs, uint32_t size) const {
		if(device) vkFreeCommandBuffers(device, commandPool, size, reinterpret_cast<VkCommandBuffer*>(cmdBufs));
	}
	const VkFence& getOTFence() const { return OTFence; }

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
	inline static VkPhysicalDeviceProperties getProperties(VkPhysicalDevice gpu) {
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(gpu, &properties);
		return properties;
	}
	inline VkPhysicalDeviceProperties getProperties() const { return getProperties(gpu); }

	inline VkFormatProperties getFormatProperties(VkFormat format) const {
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(gpu, format, &formatProperties);
		return formatProperties;
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

	template<auto getRequirements>
	VkResult allocateMemory(auto object, VkMemoryPropertyFlags properties, VkDeviceMemory &memory) const {
		VkMemoryRequirements memReq;
		getRequirements(device, object, &memReq);
		VkMemoryAllocateInfo allocInfo {
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memReq.size,
			.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, properties)
		};
		return vkAllocateMemory(device, &allocInfo, nullptr, &memory);
	}

private:
	VkPhysicalDevice gpu = nullptr;
	VkDevice device = nullptr;

	VkPhysicalDeviceFeatures features;

	QueueFamilies queueFamilies;
	VkQueue graphicsQueue, presentQueue;

	VkCommandPool commandPool;
	VkFence OTFence;
};

}