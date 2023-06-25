#pragma once

#include "device.h"

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
	void init(const Device &device, const Shader &vertexShader, const Shader &fragmentShader);
};

}