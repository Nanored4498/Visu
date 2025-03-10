#pragma once

#include "window.h"
#include "sync.h"

namespace gfx {

class Swapchain {
public:
	~Swapchain() { clean(); }

	void init(const Device &device, const Window &window);
	void recreate(const Device &device, const Window &window);
	void clean();
	void cleanOld();

	uint32_t acquireNextImage(Semaphore &signal);
	VkResult presentImage(uint32_t imIndex, VkQueue queue, Semaphore &wait);

	inline VkFormat getFormat() const { return format.format; }
	inline VkExtent2D getExtent() const { return extent; }
	inline uint32_t getWidth() const { return extent.width; }
	inline uint32_t getHeight() const { return extent.height; }
	inline std::size_t size() const { return imageViews.size(); }
	inline VkImage getImage(std::size_t i) const { return images[i]; }
	inline VkImageView getView(std::size_t i) const { return imageViews[i]; }

private:
	VkSwapchainKHR swapchain = nullptr, old = nullptr;
	VkSurfaceFormatKHR format;
	VkPresentModeKHR presentMode;
	VkExtent2D extent;
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;

	VkDevice device;

	void __init(const Device &device, const Window &window);
	void __cleanViews();
};

}