#pragma once

#include "swapchain.h"

namespace gfx {

class RenderPass {
public:
	~RenderPass() { clean(); }

	void init(const Device &device, const Swapchain &swapchain);
	void initFramebuffers(const Swapchain &swapchain);
	void clean();
	void cleanFramebuffers();

	inline operator VkRenderPass() const { return pass; }

	inline std::size_t size() const { return framebuffers.size(); }
	inline VkFramebuffer framebuffer(std::size_t i) const { return framebuffers[i]; }

protected:
	VkRenderPass pass;
	std::vector<VkFramebuffer> framebuffers;

	VkDevice device;
};

}