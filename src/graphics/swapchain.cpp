#include "swapchain.h"

#include "image.h"

#include "debug.h"

#include <algorithm>
#include <limits>

namespace gfx {

void Swapchain::init(const Device &device, const Window &window) {
	clean();

	// Image format
	constexpr VkFormat desiredFormat = VK_FORMAT_B8G8R8A8_SRGB;
	constexpr VkColorSpaceKHR desiredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	const std::vector<VkSurfaceFormatKHR> formats = device.getSurfaceFormats(window);
	format = formats[0];
	if(formats.size() > 1) for(const VkSurfaceFormatKHR &f : formats) {
		int score = int(f.format == desiredFormat) + int(f.colorSpace == desiredColorSpace);
		if(!score) continue;
		format = f;
		if(score == 2) break;
	} else if(format.format == VK_FORMAT_UNDEFINED) {
		format.format = desiredFormat;
		format.colorSpace = desiredColorSpace;
	}
	if(format.format != desiredFormat) DEBUG_MSG("WARNING: Desired image format (B8G8R8A8_SRGB) not available: using", format.format, "instead...");
	if(format.colorSpace != desiredColorSpace) DEBUG_MSG("WARNING: Desired color space (SRGB_NONLINEAR_KHR) not available: using", format.colorSpace, "instead...");

	// Present mode
	const std::vector<VkPresentModeKHR> presents = device.getPresentModes(window);
	if(std::ranges::find(presents, VK_PRESENT_MODE_MAILBOX_KHR) != presents.end()) {
		presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	} else if(std::ranges::find(presents, VK_PRESENT_MODE_FIFO_KHR) != presents.end()) {
		presentMode = VK_PRESENT_MODE_FIFO_KHR;
		DEBUG_MSG("WARNING: Desired present mode (Mailbox) not available: using FIFO instead...");
	} else /*Should never happened as FIFO is guaranted*/ {
		presentMode = presents[0];
		DEBUG_MSG("WARNING: Desired present mode (Mailbox) not available: using", presentMode,"instead...");
	}

	__init(device, window);
}

void Swapchain::__init(const Device &device, const Window &window) {
	// Extent
	const VkSurfaceCapabilitiesKHR capabilities = device.getCapabilities(window);
	if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		extent = capabilities.currentExtent;
	} else {
		int width, height;
		window.getFramebufferSize(width, height);
		while(!width || !height) {
			glfwWaitEvents();
			window.getFramebufferSize(width, height);
		}
		extent.width = std::clamp((uint32_t) width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = std::clamp((uint32_t) height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	}

	// Create the swapchain
	VkSwapchainCreateInfoKHR swapInfo {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0u,
		.surface = window.getSurface(),
		.minImageCount = capabilities.maxImageCount && capabilities.minImageCount >= capabilities.maxImageCount ?
							capabilities.maxImageCount
							: capabilities.minImageCount + 1,
		.imageFormat = format.format,
		.imageColorSpace = format.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1u, // 2 for stereoscopic 3D
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0u,
		.pQueueFamilyIndices = nullptr,
		.preTransform = capabilities.currentTransform, // To apply rotation or mirror to image
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, // To use alpha
		.presentMode = presentMode,
		.clipped = VK_TRUE, // Don't render pixels hidden by other windows
		.oldSwapchain = old
	};
	// Update sharing mode if several queues
	if(device.getQueueFamilies().graphicsId != device.getQueueFamilies().presentId) {
		swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapInfo.queueFamilyIndexCount = 2u;
		swapInfo.pQueueFamilyIndices = device.getQueueFamilies();
	}
	if(vkCreateSwapchainKHR(this->device = device, &swapInfo, nullptr, &swapchain) != VK_SUCCESS)
		THROW_ERROR("failed to create swap chain!");

	// Get swapchain image views
	images = vkGetList(vkGetSwapchainImagesKHR, (VkDevice) device, swapchain);
	imageViews.resize(images.size());
	for(std::size_t i = 0; i < images.size(); ++i)
		imageViews[i] = Image::createView(device, images[i], 2, format.format, VK_IMAGE_ASPECT_COLOR_BIT);
}

void Swapchain::recreate(const Device &device, const Window &window) {
	old = swapchain;
	__cleanViews();
	__init(device, window);
}

void Swapchain::clean() {
	cleanOld();
	if(!swapchain) return;
	__cleanViews();
	vkDestroySwapchainKHR(device, swapchain, nullptr);
	swapchain = nullptr;
}

void Swapchain::cleanOld() {
	if(!old) return;
	vkDestroySwapchainKHR(device, old, nullptr);
	old = nullptr;
}

void Swapchain::__cleanViews() {
	for(VkImageView view : imageViews)
		vkDestroyImageView(device, view, nullptr);
	imageViews.clear();
}

uint32_t Swapchain::acquireNextImage(Semaphore &signal) {
	uint32_t ind;
	switch(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, signal, VK_NULL_HANDLE, &ind)) {
	case VK_SUCCESS: case VK_SUBOPTIMAL_KHR:
		return ind;
	case VK_ERROR_OUT_OF_DATE_KHR:
		return UINT32_MAX;
	default:
		THROW_ERROR("failed to acquire swapchain image!");
	}
}

VkResult Swapchain::presentImage(uint32_t imIndex, VkQueue queue, Semaphore &wait) {
	const VkPresentInfoKHR presentInfo {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &wait,
		.swapchainCount = 1u,
		.pSwapchains = &swapchain,
		.pImageIndices = &imIndex,
		.pResults = nullptr
	};
	return vkQueuePresentKHR(queue, &presentInfo);
}

}