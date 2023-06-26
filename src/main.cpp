#include <iostream>
#include <stdexcept>

#include <config.h>
#include <graphics/renderpass.h>
#include <graphics/pipeline.h>
#include <graphics/commandbuffer.h>

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
	cmdBuffs.resize(swapchain.size());
	for(std::size_t i = 0; i < cmdBuffs.size(); ++i) {
		cmdBuffs[i].begin()
			.beginRenderPass(renderPass, i, swapchain.getExtent())
				.bindPipeline(pipeline)
				.setViewport(swapchain.getExtent())
				.draw(3, 1, 0, 0)
			.endRenderPass()
		.end();
	}
}

void loop() {
	while(!window.shouldClose()) {
		glfwPollEvents();
	}
}

void clean() {
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