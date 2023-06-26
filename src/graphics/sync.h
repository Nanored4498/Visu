#pragma once

#include "debug.h"
#include "device.h"

namespace gfx {

class Fence {
public:
	Fence() = default;
	Fence(const Device &device, bool signaled=false) { init(device, signaled); }
	~Fence() { clean(); }

	inline void init(const Device &device, bool signaled=false) {
		clean();
		const VkFenceCreateInfo fenceInfo {
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = signaled ? (VkFlags) VK_FENCE_CREATE_SIGNALED_BIT : 0u
		};
		if(vkCreateFence(this->device = device, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
			THROW_ERROR("failed to create fence!");
	}

	inline void clean() { 
		if(!fence) return;
		vkDestroyFence(device, fence, nullptr);
		fence = nullptr;
	}

	inline void wait(uint64_t timeout=UINT64_MAX) { vkWaitForFences(device, 1u, &fence, VK_TRUE, timeout); }
	inline void reset() { vkResetFences(device, 1u, &fence); }

	operator VkFence() const { return fence; }

private:
	VkFence fence = nullptr;
	VkDevice device;
};

class Semaphore {
public:
	Semaphore() = default;
	Semaphore(const Device &device) { init(device); }
	~Semaphore() { clean(); }

	inline void init(const Device &device) {
		clean();
		const VkSemaphoreCreateInfo semInfo {
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0u
		};
		if(vkCreateSemaphore(this->device = device, &semInfo, nullptr, &semaphore) != VK_SUCCESS)
			THROW_ERROR("failed to create semaphore!");
	}

	inline void clean() { 
		if(!semaphore) return;
		vkDestroySemaphore(device, semaphore, nullptr);
		semaphore = nullptr;
	}

	operator VkSemaphore() const { return semaphore; }
	VkSemaphore* operator&() { return &semaphore; }

private:
	VkSemaphore semaphore = nullptr;
	VkDevice device;
};

}