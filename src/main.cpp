#include <iostream>
#include <stdexcept>

#include <config.h>
#include <graphics/swapchain.h>

const char* APP_NAME = "Visu";
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

gfx::Instance instance;
gfx::Window window;
gfx::Device device;
gfx::Swapchain swapchain;

void init() {
	instance.init(APP_NAME, gfx::Window::getRequiredExtensions());
	window.init(APP_NAME, WIDTH, HEIGHT, instance);
	device.init(instance, window);
	swapchain.init(device, window);
}

void loop() {
	while(!window.shouldClose()) {
		glfwPollEvents();
	}
}

void clean() {
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