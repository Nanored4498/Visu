#include <iostream>
#include <stdexcept>

#include <config.h>
#include <graphics/renderpass.h>
#include <graphics/pipeline.h>
#include <graphics/commandbuffer.h>
#include <graphics/sync.h>

#include <algorithm>

const char* APP_NAME = "Visu";
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

gfx::Instance instance;
gfx::Window window;
gfx::Device device;
gfx::Swapchain swapchain;
gfx::RenderPass renderPass;
gfx::Pipeline pipeline;
gfx::CommandBuffers cmdBuffs(device);
std::vector<gfx::CommandBuffer::SubmitSync> submitSyncs;
std::size_t currentFrame = 0;

void init() {
	instance.init(APP_NAME, gfx::Window::getRequiredExtensions());
	window.init(APP_NAME, WIDTH, HEIGHT, instance);
	device.init(instance, window);
	swapchain.init(device, window);
	renderPass.init(device, swapchain);
	pipeline.init(device,
		gfx::Shader(device, SHADER_DIR "/test.vert.spv"),
		gfx::Shader(device, SHADER_DIR "/test.frag.spv"),
		renderPass
	);
	cmdBuffs.resize(renderPass.size());
	for(std::size_t i = 0; i < cmdBuffs.size(); ++i)
		cmdBuffs[i].begin()
			.beginRenderPass(renderPass, i, swapchain.getExtent())
				.bindPipeline(pipeline)
				.setViewport(swapchain.getExtent())
				.draw(3, 1, 0, 0)
			.endRenderPass()
		.end();
	
	const std::size_t nFrames = std::clamp(renderPass.size(), 1ul, 2ul);
	submitSyncs.resize(nFrames);
	for(auto &sync : submitSyncs) sync.init(device);
}

void loop() {
	while(!window.shouldClose()) {
		glfwPollEvents();
		const uint32_t imIndex = swapchain.acquireNextImage(submitSyncs[currentFrame].imageAvailable);
		if(imIndex == UINT32_MAX) {
			// Recreate swapchain
			continue;
		}
		cmdBuffs[imIndex].submit(device.getGraphicsQueue(), submitSyncs[currentFrame]);
		const VkResult result = swapchain.presentImage(imIndex, device.getPresentQueue(), submitSyncs[currentFrame].renderFinished);
		if(result != VK_SUCCESS) THROW_ERROR("failed to present swapchain image!");
		if(++ currentFrame == submitSyncs.size()) currentFrame = 0ul;
	}
}

void clean() {
	device.waitIdle();
	submitSyncs.clear();
	pipeline.clean();
	renderPass.clean();
	swapchain.clean();
	device.clean();
	window.clean();
	instance.clean();
}

int main() {
	Config::load();
	glfwInit();

	try {
		init();
		loop();
		clean();
	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	glfwTerminate();
	Config::save();

	return 0;
}