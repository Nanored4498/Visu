#include "image.h"

#include "util.h"

namespace gfx {

VkImageView Image::createView(VkDevice device, VkImage image, int dim, VkFormat format, VkImageAspectFlags aspect, uint32_t levelCount) {
	ASSERT(1 <= dim && dim <= 3);
	static constexpr VkImageViewType viewTypes[] { VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D };
	VkImageViewCreateInfo viewInfo {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.image = image,
		.viewType = viewTypes[dim-1],
		.format = format,
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange = {
			.aspectMask = aspect,
			.baseMipLevel = 0u,
			.levelCount = levelCount,
			.baseArrayLayer = 0u,
			.layerCount = 1u
		}
	};
	VkImageView view;
	if(vkCreateImageView(device, &viewInfo, nullptr, &view) != VK_SUCCESS)
		throw std::runtime_error("failed to create image view!");
	return view;
}

}