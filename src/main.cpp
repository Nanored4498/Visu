#include <iostream>
#include <stdexcept>

#include <graphics/window.h>

const char* APP_NAME = "Visu";
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

Instance instance;
Window window;

void init() {
	instance.init(APP_NAME, Window::getRequiredExtensions());
	window.init(APP_NAME, WIDTH, HEIGHT);
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

	return 0;
}