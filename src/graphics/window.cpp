#include "window.h"

void Window::init(const char* name, int width, int height) {
	clean();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(width, height, name, nullptr, nullptr);
}

void Window::clean() {
	if(!window) return;
	glfwDestroyWindow(window);
	window = nullptr;
}
