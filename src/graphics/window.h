#pragma once

#include <GLFW/glfw3.h>

#include "instance.h"

class Window {
public:
	~Window() { clean(); }

	void init(const char* name, int width, int height);
	void clean();

	inline bool shouldClose() { return glfwWindowShouldClose(window); }

	inline static Extensions getRequiredExtensions() {
		Extensions exts;
		exts.exts = glfwGetRequiredInstanceExtensions(&exts.count);
		return exts;
	}


private:
	GLFWwindow *window = nullptr;
};
