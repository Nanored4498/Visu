#pragma once

#include <vulkan/vulkan.h>

namespace gfx {

class Image {
public:
	static VkImageView createView(VkDevice device, VkImage image, int dim, VkFormat format, VkImageAspectFlags aspect, uint32_t levelCount=1u);
};

}
