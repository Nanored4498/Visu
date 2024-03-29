// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

#pragma once

#include "instance.h"

#include <GLFW/glfw3.h>

namespace gfx {

class Window {
public:
	~Window() { clean(); }

	void init(Instance &instance, const char* name, int width, int height, int x=-1, int y=-1);
	void clean();

	inline operator GLFWwindow*() const { return window; }

	inline VkSurfaceKHR getSurface() const { return surface; }
	inline bool shouldClose() const { return closeRequired | glfwWindowShouldClose(window); }
	inline bool isFramebufferResized() const { return framebufferResized; }

	inline void resetFramebufferResized() { framebufferResized = false; }
	inline void requireClose() { closeRequired = true; }

	inline void getFramebufferSize(int &width, int &height) const {
		glfwGetFramebufferSize(window, &width, &height);
	}

	inline void setScrollCallback(GLFWscrollfun callback) { glfwSetScrollCallback(window, callback); }
	inline void setMouseButtonCallback(GLFWmousebuttonfun callback) { glfwSetMouseButtonCallback(window, callback); }
	inline void setCursorPosCallback(GLFWcursorposfun callback) { glfwSetCursorPosCallback(window, callback); }
	inline void setKeyCallback(GLFWkeyfun callback) { glfwSetKeyCallback(window, callback); }

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
	
	bool framebufferResized = false;
	bool closeRequired = false;
};

}