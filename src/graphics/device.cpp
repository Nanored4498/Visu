#include "device.h"

#include "util.h"
#include "config.h"

#include <algorithm>
#include <cstring>
#include <stdint.h>
#include <vector>

static const char* RequiredExtensions[] {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
	#ifdef __APPLE__
	, VK_KHR_portability_subset
	#endif
};

void Device::init(Instance &instance, VkSurfaceKHR surface) {
	clean();
	pickPhysicalDevice(instance, surface);
	createLogicalDevice(surface);
}

void Device::clean() {
	if(!device) return;
	vkDestroyDevice(device, nullptr);
	device = nullptr;
}

static QueueFamilies findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	QueueFamilies indices;
	for(uint i = 0; i < queueFamilyCount; ++i) {
		if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsId = i;
		VkBool32 surfaceSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &surfaceSupport);
		if(surfaceSupport) {
			indices.presentId = i;
			if(indices.presentId == indices.graphicsId) return indices;
		}
	}

	return indices;
}

static VkPhysicalDeviceProperties getPhysicalDeviceProperty(VkPhysicalDevice gpu) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(gpu, &deviceProperties);
	return deviceProperties;
}

static bool isPhysicalDeviceSuitable(VkPhysicalDevice gpu, VkSurfaceKHR surface) {
	// Need geometry shader
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(gpu, &deviceFeatures);
	if(!deviceFeatures.geometryShader) return false;

	// Check queues
	QueueFamilies queueIndices = findQueueFamilies(gpu, surface);
	if(!queueIndices.correct()) return false;

	// Check extensions
	uint32_t count;
	vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr);
	std::vector<VkExtensionProperties> extensions(count);
	vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, extensions.data());
	for(const char* ext : RequiredExtensions)
		if(std::ranges::find_if(extensions, [&](const VkExtensionProperties &e) { return !strcmp(e.extensionName, ext); }) == extensions.end())
			return false;

	// Check format and present mode availability
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, nullptr);
	if(count == 0u) return false;
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count, nullptr);
	if(count == 0u) return false;

	return true;
}

void Device::pickPhysicalDevice(Instance &instance, VkSurfaceKHR surface) {
	uint32_t deviceCount = 0u;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if(!deviceCount) THROW_ERROR("failed to find a GPU with Vulkan support!");
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for(VkPhysicalDevice candidate : devices)
		if(isPhysicalDeviceSuitable(candidate, surface) && (!gpu || !strcmp(getPhysicalDeviceProperty(candidate).deviceName, Config::data.preferred_gpu)))
			gpu = candidate;

	if(!gpu) THROW_ERROR("failed to find a suitable GPU!");
	debugMessage("Using Physical Device:", getPhysicalDeviceProperty(gpu).deviceName);
}

void Device::createLogicalDevice(VkSurfaceKHR surface) {
	queueFamilies = findQueueFamilies(gpu, surface);
	std::vector<VkDeviceQueueCreateInfo> queueInfos;
	float queuePriority = 1.f;
	const auto addQ = [&](uint32_t i) {
		queueInfos.push_back({
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0u,
			.queueFamilyIndex = i,
			.queueCount = 1u,
			.pQueuePriorities = &queuePriority
		});
	};
	addQ(queueFamilies.graphicsId);
	if(queueFamilies.presentId != queueFamilies.graphicsId) addQ(queueFamilies.presentId);

	features = {};
	// VkPhysicalDeviceFeatures deviceFeatures;
	// vkGetPhysicalDeviceFeatures(gpu, &deviceFeatures);
	// features.samplerAnisotropy = deviceFeatures.samplerAnisotropy;

	VkDeviceCreateInfo deviceInfo {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.queueCreateInfoCount = (uint32_t) queueInfos.size(),
		.pQueueCreateInfos = queueInfos.data(),
		.enabledLayerCount = 0u,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = sizeof(RequiredExtensions) / sizeof(RequiredExtensions[0]),
		.ppEnabledExtensionNames = RequiredExtensions,
		.pEnabledFeatures = &features
	};
	#ifndef NDEBUG
		deviceInfo.enabledLayerCount = 1u;
		deviceInfo.ppEnabledLayerNames = &Instance::validation_layer;
	#endif
	if(vkCreateDevice(gpu, &deviceInfo, nullptr, &device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device!");
		
	// Get queues
	vkGetDeviceQueue(device, queueFamilies.graphicsId, 0, &graphicsQueue);
	vkGetDeviceQueue(device, queueFamilies.presentId, 0, &presentQueue);
}
