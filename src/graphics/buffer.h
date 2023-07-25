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

	static void copy(const Device &device, const Buffer &src, Buffer &dst, VkDeviceSize size);

	inline static Buffer createStagingBuffer(const Device &device, void* data, VkDeviceSize size) {
		Buffer buffer(
			device,
			size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		buffer.fillWithData(data, size);
		return buffer;
	}

	operator VkBuffer() const { return buffer; }

private:
	VkBuffer buffer = nullptr;
	VkDeviceMemory memory;

	VkDevice device;
};

}