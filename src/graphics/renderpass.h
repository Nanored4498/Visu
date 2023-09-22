#pragma once

#include "swapchain.h"
#include "image.h"

namespace gfx {

class RenderPass {
public:
	~RenderPass() { clean(); }

	void init(const Device &device, const Swapchain &swapchain, const DepthImage &depthImage);
	void initFramebuffers(const Swapchain &swapchain, const DepthImage &depthImage);
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