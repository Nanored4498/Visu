#include "window.h"

#include "debug.h"

namespace gfx {

void Window::init(const char* name, int width, int height, Instance &instance) {
	clean();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, name, nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	if(glfwCreateWindowSurface(this->instance = instance, window, nullptr, &surface) != VK_SUCCESS)
		THROW_ERROR("failed to create window surface!");
}

void Window::clean() {
	if(!window) return;
	vkDestroySurfaceKHR(instance, surface, nullptr);
	glfwDestroyWindow(window);
	window = nullptr;
}

///////////////
// Callbacks //
///////////////

void Window::framebufferSizeCallback(GLFWwindow *window, int, int) {
	auto app = (Window*) glfwGetWindowUserPointer(window);
	app->framebufferResized = true;
}

}