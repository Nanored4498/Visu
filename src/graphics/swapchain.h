#pragma once

#include "device.h"
#include "window.h"

namespace gfx {

class Swapchain {
public:
	~Swapchain() { clean(); }

	void init(const Device &device, const Window &window);
	void clean();

	inline VkFormat getFormat() const { return format.format; }

private:
	VkSwapchainKHR swapchain;
	VkSurfaceFormatKHR format;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
	std::vector<VkImageView> imageViews;

	VkDevice device;
};

}