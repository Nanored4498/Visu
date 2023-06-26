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
	inline uint32_t getWidth() const { return extent.width; }
	inline uint32_t getHeight() const { return extent.height; }
	inline std::size_t size() const { return imageViews.size(); }
	inline VkImageView operator[](std::size_t i) const { return imageViews[i]; }

private:
	VkSwapchainKHR swapchain;
	VkSurfaceFormatKHR format;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
	std::vector<VkImageView> imageViews;

	VkDevice device;
};

}