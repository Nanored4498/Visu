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
gfx::CommandBuffers cmdBuffs, uniCmdBuffs;
gfx::Semaphore imageAvailable[20], renderFinished[20];
std::vector<gfx::Fence> cmdSubmitted;
int currentFrame = 0;

int &width = Config::data.window_width;
int &height = Config::data.window_height;
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

//== Preferences ==//
bool preferenceOpened = false;
std::vector<VkPhysicalDevice> gpus;
char* __gpu_names = nullptr;
std::vector<const char*> gpu_names;
int chosenGPU = 0;
const char* styles[] { "Light", "Dark", "Classic" };
int &chosenStyle = Config::data.style;
//=================//

void initDevice();
void cleanDevice();

static void openPreferences() {
	preferenceOpened = !preferenceOpened;
}

static void setStyle() {
	switch(chosenStyle) {
		case 0: ImGui::StyleColorsLight(); break;
		case 1: ImGui::StyleColorsDark(); break;
		default: ImGui::StyleColorsClassic();
	}
}

static void scrollCallback([[maybe_unused]] GLFWwindow *window, [[maybe_unused]] double xoffset, double yoffset) {
	if(ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
	zoom *= std::pow(1.1, yoffset);
	cam.u *= zoom / cam.u.norm();
	cam.v *= zoom * float(width) / float(height) / cam.v.norm();
}

static void mouseButtonCallback(GLFWwindow *window, int button, int action, [[maybe_unused]] int mods) {
	if(action == GLFW_RELEASE) cursor_mode = IDLE;
	else {
		if(ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;
		if(button == GLFW_MOUSE_BUTTON_LEFT) {
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
}

static void cursorPosCallback([[maybe_unused]] GLFWwindow *window, double xpos, double ypos) {
	if(cursor_mode == MOVE) {
		cam.center = center0
			- 2 * (xpos - xclick) / (width * cam.u.norm2()) * cam.u
			+ 2 * (ypos - yclick) / (height * cam.v.norm2()) * cam.v;
	} else if(cursor_mode == ROTATE) {
		vec2 dp(xpos - xclick, yclick - ypos);
		const double len = dp.norm();
		if(len < 1e-8) {
			cam.u = zoom * u0;
			cam.v = zoom * float(width) / float(height) * v0;
		} else {
			dp /= len;
			const double theta = std::numbers::pi * len / std::sqrt(width*width + height*height);
			const vec3f a = dp.x * u0 + dp.y * v0;
			const vec3f b = cross(u0, v0);
			const vec3f c = (std::cos(theta) - 1.) * a + std::sin(theta) * b;
			cam.u = zoom * (u0 + dp.x * c);
			cam.v = zoom * float(width) / float(height) * (v0 + dp.y * c);
		}
		cam.w = cross(cam.v, cam.u).normalize();
	}
}

static void keyCallback([[maybe_unused]] GLFWwindow *window, int key, [[maybe_unused]] int scancode, int action, int mods) {
	if((mods & GLFW_MOD_CONTROL) && (action == GLFW_PRESS)) {
		switch(key) {
		case GLFW_KEY_P:
			openPreferences();
			break;
		default:
			break;
		}
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
			// TODO: copy use submit OT that wait for the command to finish: No need for sync here
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
			// TODO: copy use submit OT that wait for the command to finish: No need for sync here
			gfx::Buffer::copy(device, tmp, obj.vertexBuffer, size);
		}
	}
}

template<typename Fun>
static void myCombo(const char* label, const int nitems, const char* items[], int &choice, Fun fun) {
	if(ImGui::BeginCombo(label, items[choice])) {
		for(int i = 0; i < nitems; ++i) {
			const bool selected = i == choice;
			if(ImGui::Selectable(items[i], selected) && !selected) {
				choice = i;
				fun();
			}
			if(selected) ImGui::SetItemDefaultFocus();
		}
    	ImGui::EndCombo();
	}
}

static bool drawImGui() {
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	bool draw = true;
	// ImGui::ShowDemoWindow();

	// Main Menu Bar
	if(ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("File")) {
			if(ImGui::MenuItem("Open", "Ctrl+O")) {
				// TODO: Open mesh file (support shortcut)
				PRINT_INFO("NOT IMPLEMENTED");
			}
			ImGui::Separator();
			if(ImGui::MenuItem("Preferences", "Ctrl+P")) {
				openPreferences();
			}
			ImGui::Separator();
			if(ImGui::MenuItem("Quit")) {
				window.requireClose();
			}
			ImGui::EndMenu();
		}
		ImGui::Separator();
		ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::EndMainMenuBar();
	}

	if(preferenceOpened) {
		if(ImGui::Begin("Preferences", &preferenceOpened)) {
			myCombo("Style", std::size(styles), styles, chosenStyle, setStyle);
			myCombo("GPU", gpus.size(), gpu_names.data(), chosenGPU, [&](){ draw = false; });
			ImGui::Separator();
			if(ImGui::Button("Save")) {
				std::strcpy(Config::data.preferred_gpu, gpu_names[chosenGPU]);
				glfwGetWindowPos(window, &Config::data.window_x, &Config::data.window_y);
				Config::save();
				ImGui::GetIO().WantSaveIniSettings = true;
				ImGui::SaveIniSettingsToDisk(BUILD_DIR "/imgui.ini");
				ImGui::GetIO().WantSaveIniSettings = false;
			}
			ImGui::SameLine();
			if(ImGui::Button("Reload")) {
				Config::load();
				setStyle();
				for(int i = 0; i < (int) gpus.size(); ++i)
					if(i != chosenGPU && !strcmp(gpu_names[i], Config::data.preferred_gpu)) {
						chosenGPU = i;
						draw = false;
						break;
					}
			}
		}
		ImGui::End();
	}

	for(Object &obj :objects) {
		if(ImGui::Begin((obj.name + " properties").c_str())) {
			if(ImGui::Checkbox("Smooth Shading", &smooth_shading)) fillVertexBuffer();
			ImGui::ColorEdit3("Surface Color", obj.surfaceColor);
		}
		ImGui::End();
	}

	ImGui::Render();
	return draw;
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

void initDevice() {
	PRINT_INFO("Using Physical Device:", gpu_names[chosenGPU]);
	device.init(window, gpus[chosenGPU]);
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
	gui.init(instance, device, swapchain);
	for(Object &obj : objects) obj.vertexBuffer.init(device, sizeof(gfx::Vertex) * obj.nfacet_corners());
	fillVertexBuffer();
	cmdBuffs.init(device);
	initCmdBuffs();
	uniCmdBuffs.init(device);
	uniCmdBuffs.resize(renderPass.size());
	for(gfx::Semaphore &s : imageAvailable) s.init(device);
	for(gfx::Semaphore &s : renderFinished) s.init(device);
	cmdSubmitted.resize(cmdBuffs.size());
	for(gfx::Fence &f : cmdSubmitted) f.init(device, true);
}

void init() {
	// Vulkan instance
	instance.init(APP_NAME, gfx::Window::getRequiredExtensions());

	// Window
	window.init(instance, APP_NAME,
		width = Config::data.window_width, height = Config::data.window_height,
		Config::data.window_x, Config::data.window_y);
	window.setScrollCallback(scrollCallback);
	window.setMouseButtonCallback(mouseButtonCallback);
	window.setCursorPosCallback(cursorPosCallback);
	window.setKeyCallback(keyCallback);

	// GPUs list
	gpus = gfx::Device::getAvailableDevices(instance, window);
	gpu_names.resize(gpus.size());
	std::size_t names_size = 0;
	for(VkPhysicalDevice gpu : gpus) names_size += std::strlen(gfx::Device::getProperties(gpu).deviceName)+1;
	char* ind = __gpu_names = new char[names_size];
	for(int i = 0; i < (int) gpus.size(); ++i) {
		const VkPhysicalDeviceProperties prop = gfx::Device::getProperties(gpus[i]);
		const char* name = prop.deviceName;
		gpu_names[i] = ind;
		while(*name) *(ind++) = *(name++);
		*(ind++) = *(name++);
		if(!strcmp(gpu_names[i], Config::data.preferred_gpu)) chosenGPU = i;
	}

	// Imgui init
	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = nullptr;
	ImGui::LoadIniSettingsFromDisk(BUILD_DIR "/imgui.ini");
	setStyle();
	ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 16.f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
  	ImGui_ImplGlfw_InitForVulkan(window, true);

	// Init device
	initDevice();
}

void updateSwapchain() {
	window.getFramebufferSize(width, height);
	while(!width || !height) {
		glfwWaitEvents();
		window.getFramebufferSize(width, height);
	}
	window.resetFramebufferResized();
	cam.v *= zoom * float(width) / float(height) / cam.v.norm();
	device.waitIdle();
	renderPass.cleanFramebuffers();
	swapchain.recreate(device, window);
	depthImage.recreate(device, swapchain.getExtent());
	renderPass.initFramebuffers(swapchain, depthImage);
	gui.update(swapchain);
	//TODO: Maybe descriptor pool should be resized
	initCmdBuffs();
	uniCmdBuffs.resize(renderPass.size());
	cmdSubmitted.resize(cmdBuffs.size());
	for(gfx::Fence &f : cmdSubmitted) if(!f) f.init(device, true);
	swapchain.cleanOld();
}

long long rendered_frames = 0;
void loop() {
	while(!window.shouldClose()) {
		const uint32_t imIndex = swapchain.acquireNextImage(imageAvailable[currentFrame]);
		if(imIndex == UINT32_MAX) {
			updateSwapchain();
			continue;
		}
		cmdSubmitted[imIndex].wait();
		cmdSubmitted[imIndex].reset();
		glfwPollEvents();
		if(!drawImGui()) {
			cleanDevice();
			initDevice();
			continue;
		}
		// TODO: Look at secondary command buffers instead of OT
		uniCmdBuffs[imIndex].beginOT()
			.updateBuffer(descriptorPool.getBuffer(), descriptorPool.getOffset(imIndex, 0), sizeof(cam), &cam)
			.bufferBarrier(descriptorPool.getBuffer(), descriptorPool.getOffset(imIndex, 0), sizeof(cam))
		.end();
		gfx::CommandBuffer cmds[] { uniCmdBuffs[imIndex], cmdBuffs[imIndex], gui.getCommand(swapchain, imIndex) };
		gfx::CommandBuffer::submit(cmds, std::size(cmds),
								device.getGraphicsQueue(),
								imageAvailable[currentFrame], renderFinished[currentFrame],
								cmdSubmitted[imIndex]);
		const VkResult result = swapchain.presentImage(imIndex, device.getPresentQueue(), renderFinished[currentFrame]);
		if(window.isFramebufferResized() || result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			updateSwapchain();
		else if(result != VK_SUCCESS) THROW_ERROR("failed to present swapchain image!");
		++ rendered_frames;
		currentFrame = (currentFrame + 1) % 20;
	}
}

void cleanDevice() {
	device.waitIdle();
	cmdSubmitted.clear();
	for(gfx::Semaphore &s : renderFinished) s.clean();
	for(gfx::Semaphore &s : imageAvailable) s.clean();
	uniCmdBuffs.clear();
	cmdBuffs.clear();
	for(Object &obj : objects) obj.vertexBuffer.clean();
	gui.clean();
	pipeline.clean();
	descriptorPool.clean();
	renderPass.clean();
	depthImage.clean();
	swapchain.clean();
	device.clean();
}

void clean() {
	cleanDevice();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
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

	return 0;
}