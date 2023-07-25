#include "buffer.h"

#include "debug.h"

namespace gfx {

void Buffer::init(const Device &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
	clean();
	
	// Create buffer
	VkBufferCreateInfo bufferInfo {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0u,
		.pQueueFamilyIndices = nullptr
	};
	if(vkCreateBuffer(this->device = device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		THROW_ERROR("failed to create buffer!");

	// Memory allocation
	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(device, buffer, &memReq);
	VkMemoryAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memReq.size,
		.memoryTypeIndex = device.findMemoryType(memReq.memoryTypeBits, properties)
	};
	if(vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS)
		THROW_ERROR("failed to allocate buffer memory!");
	vkBindBufferMemory(device, buffer, memory, 0u);
}

void Buffer::clean() {
	if(!buffer) return;
	vkDestroyBuffer(device, buffer, nullptr);
	vkFreeMemory(device, memory, nullptr);
	buffer = nullptr;
}

void copyBuffer(Device &device, Buffer &src, Buffer &dst, VkDeviceSize size) {
	/*
	VkCommandBuffer cmdBuf = device.createOneTimeCommandBuffer();
	const VkBufferCopy region {
		.srcOffset = 0u,
		.dstOffset = 0u,
		.size = size
	};
	vkCmdCopyBuffer(cmdBuf, src, dst, 1u, &region);
	device.submitOneTimeCommandBuffer(cmdBuf);
	*/
}

}