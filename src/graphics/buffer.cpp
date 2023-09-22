#include "buffer.h"

#include "debug.h"
#include "commandbuffer.h"

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
	if(device.allocateMemory<vkGetBufferMemoryRequirements>(buffer, properties, memory) != VK_SUCCESS)
		THROW_ERROR("failed to allocate buffer memory!");
	vkBindBufferMemory(device, buffer, memory, 0u);
}

void Buffer::clean() {
	if(!buffer) return;
	vkDestroyBuffer(device, buffer, nullptr);
	vkFreeMemory(device, memory, nullptr);
	buffer = nullptr;
}

void Buffer::copy(const Device &device, const Buffer &src, Buffer &dst, VkDeviceSize size) {
	device.createCommandBuffer().beginOT().copyBuffer(src, dst, size).end().submitOT(device, device.getGraphicsQueue());
}

}