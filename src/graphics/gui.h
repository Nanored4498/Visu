#pragma once

#include "renderpass.h"
#include "commandbuffer.h"

namespace gfx {

class GUIRenderPass : public RenderPass {
public:
	void init(const Device &device, const Swapchain &swapchain);
	void initFramebuffers(const Swapchain &swapchain);
};

class GUI {
public:
	~GUI() { clean(); }

	void init(Instance &instance, const Device &device, const Window &window, const Swapchain &swapchain);
	void clean();

	void update(const Swapchain &swapchain);
	CommandBuffer& getCommand(const Swapchain &swapchain, uint32_t i);

private:
	VkDescriptorPool descriptorPool = nullptr;
	GUIRenderPass renderPass;
	CommandBuffers cmdBufs;
	VkDevice device;
};

}