#include "device.h"

#include "util.h"
#include "config.h"

#include <algorithm>
#include <cstring>
#include <stdint.h>
#include <vector>

void Device::init(Instance &instance) {
	clean();
	pickPhysicalDevice(instance);
}

void Device::clean() {
	if(!gpu) return;
}

static VkPhysicalDeviceProperties getPhysicalDeviceProperty(VkPhysicalDevice gpu) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(gpu, &deviceProperties);
	return deviceProperties;
}

static bool isPhysicalDeviceSuitable(VkPhysicalDevice gpu) {
	// Need geometry shader
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(gpu, &deviceFeatures);
	if(!deviceFeatures.geometryShader) return false;

	// Check queues
	// QueueFamilyIndices queueIndices = findQueueFamilyIndices(gpu, surface);
	// if(!queueIndices.correct()) return false;

	// Check extensions (SWAPCHAIN)
	uint32_t count;
	vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, nullptr);
	std::vector<VkExtensionProperties> extensions(count);
	vkEnumerateDeviceExtensionProperties(gpu, nullptr, &count, extensions.data());
	if(std::ranges::find_if(extensions, [&](const VkExtensionProperties &e) {
			return !strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}) == extensions.end()) return false;

	// Check format and present mode availability
	// vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &count, nullptr);
	// if(count == 0u) return false;
	// vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &count, nullptr);
	// if(count == 0u) return false;

	return true;
}

void Device::pickPhysicalDevice(Instance &instance) {
	uint32_t deviceCount = 0u;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if(!deviceCount) THROW_ERROR("failed to find a GPU with Vulkan support!");
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for(VkPhysicalDevice candidate : devices)
		if(isPhysicalDeviceSuitable(candidate) && (!gpu || !strcmp(getPhysicalDeviceProperty(candidate).deviceName, Config::data.preferred_gpu)))
			gpu = candidate;

	if(!gpu) THROW_ERROR("failed to find a suitable GPU!");
	debugMessage("Using Physical Device:", getPhysicalDeviceProperty(gpu).deviceName);
}