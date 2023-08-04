// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

#pragma once

#include "renderpass.h"
#include "descriptor.h"

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

	void init(const Device &device, const Shader &vertexShader, const Shader &fragmentShader,
				const DescriptorPool &descriptorPool, const RenderPass &renderPass);
	void clean();

	inline operator VkPipeline() const { return pipeline; }
	inline VkPipelineLayout getLayout() const { return layout; }

private:
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkDevice device;
};

}