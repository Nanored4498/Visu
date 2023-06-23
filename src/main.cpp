#include <iostream>
#include <stdexcept>

#include <config.h>
#include <graphics/window.h>
#include <graphics/device.h>

const char* APP_NAME = "Visu";
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

Instance instance;
Window window;
Device device;

void init() {
	instance.init(APP_NAME, Window::getRequiredExtensions());
	window.init(APP_NAME, WIDTH, HEIGHT);
	device.init(instance);
}

void loop() {
	while(!window.shouldClose()) {
		glfwPollEvents();
	}
}

void clean() {
	window.clean();
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