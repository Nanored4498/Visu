#include "window.h"

#include "debug.h"

namespace gfx {

void Window::init(const char* name, int width, int height, Instance &instance) {
	clean();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(width, height, name, nullptr, nullptr);
	if(glfwCreateWindowSurface(this->instance = instance, window, nullptr, &surface) != VK_SUCCESS)
		THROW_ERROR("failed to create window surface!");
}

void Window::clean() {
	if(!window) return;
	if(surface) {
		vkDestroySurfaceKHR(instance, surface, nullptr);
		surface = nullptr;
	}
	glfwDestroyWindow(window);
	window = nullptr;
}

}