#pragma once

#include "instance.h"

#include <GLFW/glfw3.h>

class Window {
public:
	~Window() { clean(); }

	void init(const char* name, int width, int height, Instance &instance);
	void clean();

	inline VkSurfaceKHR getSurface() { return surface; }
	inline bool shouldClose() { return glfwWindowShouldClose(window); }

	inline static Extensions getRequiredExtensions() {
		Extensions exts;
		exts.exts = glfwGetRequiredInstanceExtensions(&exts.count);
		return exts;
	}


private:
	GLFWwindow *window = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkInstance instance;
};
