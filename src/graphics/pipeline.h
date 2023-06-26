#pragma once

#include "renderpass.h"

namespace gfx {

class Shader {
public:
	Shader(const Device &device, const char* name);
	~Shader() { clean(); }
	void clean();
	inline operator VkShaderModule() const { return shader; }
private:
	VkShaderModule shader = nullptr;
	VkDevice device;
};

class Pipeline {
public:
	~Pipeline() { clean(); }

	void init(const Device &device, const Shader &vertexShader, const Shader &fragmentShader, const RenderPass &renderPass);
	void clean();

	inline operator VkPipeline() const { return pipeline; }

private:
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkDevice device;
};

}