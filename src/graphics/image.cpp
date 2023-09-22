#include "image.h"

#include "debug.h"

namespace gfx {

void Image::init(const Device &device, VkExtent2D extent, VkImageTiling tiling, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties) {
	clean();

	const VkImageCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = VkExtent3D {
			.width = extent.width,
			.height = extent.height,
			.depth = 1u
		},
		.mipLevels = 1u,
		.arrayLayers = 1u,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0u,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	if(vkCreateImage(this->device = device, &info, nullptr, &image) != VK_SUCCESS)
		THROW_ERROR("failed to create image!");
	
	if(device.allocateMemory<vkGetImageMemoryRequirements>(image, properties, memory) != VK_SUCCESS)
		THROW_ERROR("failed to allocate image memory!");
	vkBindImageMemory(device, image, memory, 0u);
}

void Image::clean() {
	if(!image) return;
	vkFreeMemory(device, memory, nullptr);
	vkDestroyImage(device, image, nullptr);
	image = nullptr;
}

VkImageView Image::createView(VkDevice device, VkImage image, int dim, VkFormat format, VkImageAspectFlags aspect, uint32_t levelCount) {
	ASSERT(1 <= dim && dim <= 3);
	static constexpr VkImageViewType viewTypes[] { VK_IMAGE_VIEW_TYPE_1D, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_VIEW_TYPE_3D };
	const VkImageViewCreateInfo viewInfo {
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
		THROW_ERROR("failed to create image view!");
	return view;
}

void DepthImage::init(const Device &device, VkExtent2D extent) {
	clean();

	// TODO: The format should be computed just for the first init
	format = VK_FORMAT_UNDEFINED;
	for(const VkFormat f : { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }) {
		const VkFormatProperties properties = device.getFormatProperties(f);
		if(properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			format = f;
			break;
		}
	}
	if(format == VK_FORMAT_UNDEFINED)
		THROW_ERROR("failed to find a supported depth format!");

	// TODO: Reallocation is certainly expensive find a new way to recreate the depth image.
	Image::init(device, extent, VK_IMAGE_TILING_OPTIMAL, format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	view = createView(device, image, 2, format, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void DepthImage::clean() {
	if(!image) return;
	vkDestroyImageView(device, view, nullptr);
	Image::clean();
}

}