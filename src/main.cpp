#include <iostream>
#include <stdexcept>

#include <config.h>

#include <graphics/renderpass.h>
#include <graphics/pipeline.h>
#include <graphics/commandbuffer.h>
#include <graphics/sync.h>
#include <graphics/gui.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#define LUA_BINDER_IMPL
#include <lua/luabinder.h>
#include <lua/ultimaille.h>

#include <ultimaille/io/by_extension.h>

#include <algorithm>

const char* APP_NAME = "Visu";
constexpr int WIDTH = 800;
constexpr int HEIGHT = 600;

gfx::Instance instance;
gfx::Window window;
gfx::Device device;
gfx::Swapchain swapchain;
gfx::RenderPass renderPass;
gfx::Pipeline pipeline;
gfx::GUI gui;
gfx::CommandBuffers cmdBuffs;
gfx::Semaphore imageAvailable[2], renderFinished[2];
std::vector<gfx::Fence> cmdSubmitted;
int currentFrame = 0;


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
  if (ImGui::Button("Button")) {
    counter++;
  }
  ImGui::SameLine();
  ImGui::Text("counter = %d", counter);

  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
              ImGui::GetIO().Framerate);
  ImGui::End();

  ImGui::Render();
}

void initCmdBuffs() {
	cmdBuffs.resize(renderPass.size());
	for(std::size_t i = 0; i < cmdBuffs.size(); ++i)
		cmdBuffs[i].begin()
			.beginRenderPass(renderPass, swapchain, i)
				.bindPipeline(pipeline)
				.setViewport(swapchain.getExtent())
				.draw(3, 1, 0, 0)
			.endRenderPass()
		.end();
}

void init() {
	instance.init(APP_NAME, gfx::Window::getRequiredExtensions());
	window.init(APP_NAME, WIDTH, HEIGHT, instance);
	device.init(instance, window);
	swapchain.init(device, window);
	renderPass.init(device, swapchain);
	pipeline.init(device,
		gfx::Shader(device, SHADER_DIR "/test.vert.spv"),
		gfx::Shader(device, SHADER_DIR "/test.frag.spv"),
		renderPass
	);
	gui.init(instance, device, window, swapchain);
	cmdBuffs.init(device);
	initCmdBuffs();
	for(gfx::Semaphore &s : imageAvailable) s.init(device);
	for(gfx::Semaphore &s : renderFinished) s.init(device);
	cmdSubmitted.resize(cmdBuffs.size());
	for(gfx::Fence &f : cmdSubmitted) f.init(device, true);
}

void updateSwapchain() {
	int w, h;
	window.getFramebufferSize(w, h);
	while(!w || !h) {
		window.getFramebufferSize(w, h);
		glfwWaitEvents();
	} 
	window.resetFramebufferResized();
	device.waitIdle();
	swapchain.recreate(device, window);
	renderPass.initFramebuffers(swapchain);
	gui.update(swapchain);
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
	gui.clean();
	pipeline.clean();
	renderPass.clean();
	swapchain.clean();
	device.clean();
	window.clean();
	instance.clean();
}

int main(int argc, const char* argv[]) {
	Config::load();
	glfwInit();

	Lua::new_state();
	Lua::bindUltimaille();

	std::vector<std::pair<UM::SurfaceAttributes, UM::Triangles>> meshes;
	Lua::setGlobal("meshes", meshes);
	for(int i = 1; i < argc; ++i) {
		meshes.emplace_back();
		meshes.back().first = UM::read_by_extension(argv[i], meshes.back().second);
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