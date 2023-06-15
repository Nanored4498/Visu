#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

constexpr uint32_t WIDTH = 800; 
constexpr uint32_t HEIGHT = 600; 
GLFWwindow *window = nullptr;
void initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "Visu", nullptr, nullptr);
}

void initVulkan() {

}

void loop() {
	while(!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}
}

void cleanup() {
	glfwDestroyWindow(window);
	window = nullptr;
	glfwTerminate();
}

int main() {
	try {
		initWindow();
		initVulkan();
		loop();
		cleanup();
	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	return 0;
}