#pragma once

#include "device.h"

#include <cstring>

namespace gfx {

class Buffer {
public:
	Buffer() = default;
	Buffer(const Device &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
		init(device, size, usage, properties);
	}
	~Buffer() { clean(); }

	void init(const Device &device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void clean();

	inline void* mapMemory(VkDeviceSize size = VK_WHOLE_SIZE) {
		void* data;
		vkMapMemory(device, memory, 0u, size, 0u, &data);
		return data;
	}
	inline void unmapMemory() { vkUnmapMemory(device, memory); }
	inline void fillWithData(const void* data, VkDeviceSize size) {
		memcpy(mapMemory(size), data, (size_t) size);
		unmapMemory();
	}

	friend void copyBuffer(Device &device, Buffer &src, Buffer &dst, VkDeviceSize size);

	operator VkBuffer() const { return buffer; }

private:
	VkBuffer buffer = nullptr;
	VkDeviceMemory memory;

	VkDevice device;
};

}