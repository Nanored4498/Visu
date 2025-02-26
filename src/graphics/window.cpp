#include "window.h"

#include "debug.h"

namespace gfx {

void Window::init(Instance &instance, const char* name, int width, int height, int x, int y) {
	clean();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, name, nullptr, nullptr);
	if(!glfwVulkanSupported())
		THROW_ERROR("GLFW does not support Vulkan...");
	glfwSetWindowPos(window, x, y);
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