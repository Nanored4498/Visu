#pragma once

#include "instance.h"

#include <GLFW/glfw3.h>

namespace gfx {

class Window {
public:
	~Window() { clean(); }

	void init(const char* name, int width, int height, Instance &instance);
	void clean();

	inline VkSurfaceKHR getSurface() const { return surface; }
	inline bool shouldClose() const { return glfwWindowShouldClose(window); }
	inline bool isFramebufferResized() const { return framebufferResized; }

	inline void resetFramebufferResized() { framebufferResized = false; }

	inline void getFramebufferSize(int &width, int &height) const {
		glfwGetFramebufferSize(window, &width, &height);
	}

	inline static Extensions getRequiredExtensions() {
		Extensions exts;
		exts.exts = glfwGetRequiredInstanceExtensions(&exts.count);
		return exts;
	}


private:
	static void framebufferSizeCallback(GLFWwindow *window, int width, int height);

	GLFWwindow *window = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkInstance instance;
	
	bool framebufferResized;
};

}