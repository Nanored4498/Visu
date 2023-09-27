#pragma once

#include "device.h"

namespace gfx {

class Image {
public:
	~Image() { clean(); }

	void init(const Device &device, VkExtent2D extent, VkImageTiling tiling, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	void clean();

	inline operator VkImage() const { return image; }

	// TODO: used dim as a template? 
	static VkImageView createView(VkDevice device, VkImage image, int dim, VkFormat format, VkImageAspectFlags aspect, uint32_t levelCount=1u);

protected:
	VkImage image = nullptr;
	VkDeviceMemory memory;
	VkDevice device;
};

class DepthImage : Image {
public:
	~DepthImage() { clean(); }

	void init(const Device &device, VkExtent2D extent);
	void recreate(const Device &device, VkExtent2D extent);
	void clean();

	VkFormat getFormat() const { return format; }
	VkImageView getView() const { return view; }

private:
	VkFormat format;
	VkImageView view;
};

}
