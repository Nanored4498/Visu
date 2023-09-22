// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>#include <iostream>

#include <stdexcept>

#include <config.h>

#include <graphics/renderpass.h>
#include <graphics/pipeline.h>
#include <graphics/commandbuffer.h>
#include <graphics/sync.h>
#include <graphics/gui.h>
#include <graphics/vertexbuffer.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#define LUA_BINDER_IMPL
#include <lua/luabinder.h>

#include <geometry/mesh.h>

#include <algorithm>
#include <numbers>

const char* APP_NAME = "Visu";
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

struct Object {
	std::string name;
	Mesh mesh;
	// TODO: merge memory of buffers in one allocation and maybe merge buffers and use offset
	gfx::VertexBuffer vertexBuffer;
};
std::vector<Object> objects;

gfx::Instance instance;
gfx::Window window;
gfx::Device device;
gfx::Swapchain swapchain;
gfx::DepthImage depthImage;
gfx::RenderPass renderPass;
gfx::DescriptorPool descriptorPool;
gfx::Pipeline pipeline;
gfx::GUI gui;
gfx::CommandBuffers cmdBuffs;
gfx::Semaphore imageAvailable[2], renderFinished[2];
std::vector<gfx::Fence> cmdSubmitted;
int currentFrame = 0;

int width = WIDTH, height = HEIGHT;
float zoom = 1.f;
enum {
	IDLE,
	MOVE,
	ROTATE
} cursor_mode;
double xclick, yclick;
vec3f center0, u0, v0;
struct Camera {
	alignas(16) vec3f center, u, v;
} cam {
	vec3f(0, 0, 0),
	vec3f(zoom, 0, 0),
	vec3f(0, zoom * float(width) / float(height), 0)
};

static void scrollCallback([[maybe_unused]] GLFWwindow *window, [[maybe_unused]] double xoffset, double yoffset) {
	zoom *= std::pow(1.1, yoffset);
	cam.u *= zoom / cam.u.norm();
	cam.v *= zoom * float(width) / float(height) / cam.v.norm();
}

static void mouseButtonCallback(GLFWwindow *window, int button, int action, [[maybe_unused]] int mods) {
	if(action == GLFW_RELEASE) cursor_mode = IDLE;
	else if(button == GLFW_MOUSE_BUTTON_LEFT) {
		cursor_mode = MOVE;
		center0 = cam.center;
		glfwGetCursorPos(window, &xclick, &yclick);
	} else if(button == GLFW_MOUSE_BUTTON_RIGHT) {
		cursor_mode = ROTATE;
		(u0 = cam.u).normalize();
		(v0 = cam.v).normalize();
		glfwGetCursorPos(window, &xclick, &yclick);
	}
}

static void cursorPosCallback([[maybe_unused]] GLFWwindow *window, double xpos, double ypos) {
	if(cursor_mode == MOVE) {
		cam.center = center0
			- 2 * (xpos - xclick) / (width * cam.u.norm2()) * cam.u
			+ 2 * (ypos - yclick) / (height * cam.v.norm2()) * cam.v;
	} else if(cursor_mode == ROTATE) {
		vec2 dp(xpos - xclick, yclick - ypos);
		const double len = dp.norm();
		dp /= len;
		const double theta = std::numbers::pi * len / std::sqrt(width*width + height*height);
		const vec3f a = dp.x * u0 + dp.y * v0;
		const vec3f b = cross(u0, v0);
		const vec3f c = (std::cos(theta) - 1.) * a + std::sin(theta) * b;
		cam.u = zoom * (u0 + dp.x * c);
		cam.v = zoom * float(width) / float(height) * (v0 + dp.y * c);
	}
}

static void drawImGui() {
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	static float f = 0.0f;
	static int counter = 0;

	ImGui::Begin("Renderer Options");
	ImGui::Text("This is some useful text.");
	ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
	if(ImGui::Button("Button")) counter++;
	ImGui::SameLine();
	ImGui::Text("counter = %d", counter);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
				ImGui::GetIO().Framerate);
	ImGui::End();

	ImGui::Render();
}

void initCmdBuffs() {
	//TODO: If we record command buffers for every frame then use push constants
	cmdBuffs.resize(renderPass.size());
	for(std::size_t i = 0; i < cmdBuffs.size(); ++i) {
		cmdBuffs[i].begin()
			.beginRenderPass(renderPass, swapchain, i)
				.bindPipeline(pipeline)
				.setViewport(swapchain.getExtent())
				.bindDescriptorSet(pipeline, descriptorPool[i]);
				for(const Object &obj : objects) cmdBuffs[i]
					.bindVertexBuffer(obj.vertexBuffer)
					.draw(obj.mesh.nfacet_corners(), 1, 0, 0);
		cmdBuffs[i].endRenderPass().end();
	}
}

void init() {
	instance.init(APP_NAME, gfx::Window::getRequiredExtensions());
	window.init(APP_NAME, WIDTH, HEIGHT, instance);
	window.setScrollCallback(scrollCallback);
	window.setMouseButtonCallback(mouseButtonCallback);
	window.setCursorPosCallback(cursorPosCallback);
	device.init(instance, window);
	swapchain.init(device, window);
	depthImage.init(device, swapchain.getExtent());
	renderPass.init(device, swapchain, depthImage);
	descriptorPool.addUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT, sizeof(cam));
	descriptorPool.init(device, renderPass.size());
	pipeline.init(device,
		gfx::Shader(device, SHADER_DIR "/test.vert.spv"),
		gfx::Shader(device, SHADER_DIR "/test.frag.spv"),
		descriptorPool,
		renderPass
	);
	gui.init(instance, device, window, swapchain);
	for(Object &obj : objects) {
		const VkDeviceSize size = sizeof(gfx::Vertex) * obj.mesh.nfacet_corners();
		obj.vertexBuffer.init(device, size);
		gfx::Buffer tmp = gfx::Buffer::createStagingBuffer(device, size);
		gfx::Vertex* vmap = (gfx::Vertex*) tmp.mapMemory();
		for(std::size_t i = 0; i < obj.mesh.nfacet_corners(); ++i) {
			vmap[i].pos = obj.mesh.points[obj.mesh.facet_vertices[i]];
			if(obj.mesh.facet_corner_attribute.empty()) vmap[i].color = vec3f(0.);
			else vmap[i].color = obj.mesh.facet_corner_attribute[0].uv[i];
		}
		gfx::Buffer::copy(device, tmp, obj.vertexBuffer, size);
	}
	cmdBuffs.init(device);
	initCmdBuffs();
	for(gfx::Semaphore &s : imageAvailable) s.init(device);
	for(gfx::Semaphore &s : renderFinished) s.init(device);
	cmdSubmitted.resize(cmdBuffs.size());
	for(gfx::Fence &f : cmdSubmitted) f.init(device, true);
}

void updateSwapchain() {
	window.getFramebufferSize(width, height);
	while(!width || !height) {
		window.getFramebufferSize(width, height);
		glfwWaitEvents();
	}
	cam.v *= zoom * float(width) / float(height) / cam.v.norm();
	window.resetFramebufferResized();
	device.waitIdle();
	swapchain.recreate(device, window);
	depthImage.init(device, swapchain.getExtent());
	renderPass.initFramebuffers(swapchain, depthImage);
	gui.update(swapchain);
	//TODO: Maybe descriptor pool should be resized
	initCmdBuffs();
	cmdSubmitted.resize(cmdBuffs.size());
	for(gfx::Fence &f : cmdSubmitted) if(!f) f.init(device, true);
	swapchain.cleanOld();
}

void loop() {
	while(!window.shouldClose()) {
		glfwPollEvents();
		drawImGui();
		const uint32_t imIndex = swapchain.acquireNextImage(imageAvailable[currentFrame]);
		if(imIndex == UINT32_MAX) {
			updateSwapchain();
			continue;
		}
		cmdSubmitted[imIndex].wait();
		cmdSubmitted[imIndex].reset();
		* (Camera*) descriptorPool.getUniformMap(imIndex, 0) = cam;
		gfx::CommandBuffer cmds[] { cmdBuffs[imIndex], gui.getCommand(swapchain, imIndex) };
		gfx::CommandBuffer::submit(cmds, std::size(cmds),
								device.getGraphicsQueue(),
								imageAvailable[currentFrame], renderFinished[currentFrame],
								cmdSubmitted[imIndex]);
		const VkResult result = swapchain.presentImage(imIndex, device.getPresentQueue(), renderFinished[currentFrame]);
		if(window.isFramebufferResized() || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			updateSwapchain();
		else if(result != VK_SUCCESS) THROW_ERROR("failed to present swapchain image!");
		currentFrame ^= 1;
	}
}

void clean() {
	device.waitIdle();
	cmdSubmitted.clear();
	for(gfx::Semaphore &s : renderFinished) s.clean();
	for(gfx::Semaphore &s : imageAvailable) s.clean();
	for(Object &obj : objects) obj.vertexBuffer.clean();
	gui.clean();
	pipeline.clean();
	descriptorPool.clean();
	renderPass.clean();
	depthImage.clean();
	swapchain.clean();
	device.clean();
	window.clean();
	instance.clean();
}

int main(int argc, const char* argv[]) {
	Config::load();
	glfwInit();

	Lua::new_state();

	for(int i = 1; i < argc; ++i) {
		objects.emplace_back(argv[i], readMesh(argv[i]));
	}

	try {
		init();
		loop();
	} catch(const std::exception &e) {
		std::cerr << e.what() << std::endl;
		clean();
		glfwTerminate();
		return 1;
	}

	clean();
	glfwTerminate();
	Config::save();

	return 0;
}