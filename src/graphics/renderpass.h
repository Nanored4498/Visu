#pragma once

#include "swapchain.h"

namespace gfx {

class RenderPass {
public:
	~RenderPass() { clean(); }

	void init(const Device &device, const Swapchain &swapchain);
	void clean();

	inline operator VkRenderPass() const { return pass; }

private:
	VkRenderPass pass;
	std::vector<VkFramebuffer> framebuffers;

	VkDevice device;
};

}