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
#include <filesystem>

const char* APP_NAME = "Visu";
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

class Object : public Mesh {
public:
	std::string name;
	// TODO: merge memory of buffers in one allocation and maybe merge buffers and use offset
	gfx::VertexBuffer vertexBuffer;
	float surfaceColor[3];
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
	alignas(16) vec3f center, u, v, w;
} cam {
	vec3f(0, 0, 0),
	vec3f(zoom, 0, 0),
	vec3f(0, zoom * float(width) / float(height), 0),
	vec3f(0, 0, -1)
};
bool smooth_shading = false;

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
		cam.w = cross(cam.v, cam.u).normalize();
	}
}

void fillVertexBuffer() {
	if(smooth_shading) {
		for(Object &obj : objects) {
			std::vector<vec3> normals(obj.nverts(), vec3(0.));
			for(std::uint32_t f = 0, fc = 0; f < obj.nfacets(); ++f) for(; fc < obj.facet_offset[f+1]; ++fc) {
				const std::uint32_t pfc = obj.prev(f, fc);
				const std::uint32_t nfc = obj.next(f, fc);
				const vec3 a = obj.corner_point(pfc) - obj.corner_point(fc);
				const vec3 b = obj.corner_point(nfc) - obj.corner_point(fc);
				const vec3 normal = cross(a, b);
				const double c = a * b;
				const double s = normal.norm();
				const double angle = std::atan2(s, c);
				const std::uint32_t v = obj.facet_vertices[fc];
				normals[v] += (angle / s) * normal;
			}
			for(vec3 &n : normals) n.normalize();

			const VkDeviceSize size = sizeof(gfx::Vertex) * obj.nfacet_corners();
			gfx::Buffer tmp = gfx::Buffer::createStagingBuffer(device, size);
			gfx::Vertex* vmap = (gfx::Vertex*) tmp.mapMemory();
			for(std::uint32_t f = 0, fc = 0; f < obj.nfacets(); ++f) for(; fc < obj.facet_offset[f+1]; ++fc) {
				const std::uint32_t v = obj.facet_vertices[fc];
				vmap[fc].pos = obj.points[v];
				vmap[fc].normal = normals[v];
				if(obj.facet_corner_attributes.empty()) vmap[fc].uv = vec2f(0.);
				else vmap[fc].uv = obj.facet_corner_attributes[0].uv[fc];
			}
			gfx::Buffer::copy(device, tmp, obj.vertexBuffer, size);
		}
	} else {
		for(Object &obj : objects) {
			const VkDeviceSize size = sizeof(gfx::Vertex) * obj.nfacet_corners();
			gfx::Buffer tmp = gfx::Buffer::createStagingBuffer(device, size);
			gfx::Vertex* vmap = (gfx::Vertex*) tmp.mapMemory();
			for(std::uint32_t f = 0, fc = 0; f < obj.nfacets(); ++f) for(; fc < obj.facet_offset[f+1]; ++fc) {
				const std::uint32_t v = obj.facet_vertices[fc];
				vmap[fc].pos = obj.points[v];
				vmap[fc].normal = obj.corner_normal(f, fc);
				if(obj.facet_corner_attributes.empty()) vmap[fc].uv = vec2f(0.);
				else vmap[fc].uv = obj.facet_corner_attributes[0].uv[fc];
			}
			gfx::Buffer::copy(device, tmp, obj.vertexBuffer, size);
		}
	}
}

static void drawImGui() {
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	for(Object &obj :objects) {
		ImGui::Begin((obj.name + " properties").c_str());
		if(ImGui::Checkbox("Smooth Shading", &smooth_shading)) fillVertexBuffer();
		ImGui::ColorEdit3("Surface Color", obj.surfaceColor);
		ImGui::End();
	}

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
					.draw(obj.nfacet_corners(), 1, 0, 0);
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
	descriptorPool.addUniformBuffer(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(cam));
	descriptorPool.init(device, renderPass.size());
	pipeline.init(device,
		gfx::Shader(device, SHADER_DIR "/test.vert.spv"),
		gfx::Shader(device, SHADER_DIR "/test.frag.spv"),
		descriptorPool,
		renderPass
	);
	gui.init(instance, device, window, swapchain);
	for(Object &obj : objects) obj.vertexBuffer.init(device, sizeof(gfx::Vertex) * obj.nfacet_corners());
	fillVertexBuffer();
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
	depthImage.recreate(device, swapchain.getExtent());
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
		objects.emplace_back(readMesh(argv[i]));
		objects.back().name = std::filesystem::path(argv[i]).filename().replace_extension();
		std::fill_n(objects.back().surfaceColor, 3, 0.65f);
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