#include "pipeline.h"

#include "util.h"

#include <fstream>

namespace gfx {

Shader::Shader(const Device &device, const char* shaderPath) {
	// Read file
	std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);
	if(file.fail()) THROW_ERROR(std::string("Can't read file: ") + shaderPath);
	std::vector<char> code((size_t) file.tellg());
	file.seekg(0);
	file.read(code.data(), code.size());
	file.close();
	// Create shader
	VkShaderModuleCreateInfo shaderInfo {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.codeSize = code.size(),
		.pCode = (const uint*) code.data()
	};
	if(vkCreateShaderModule(this->device = device, &shaderInfo, nullptr, &shader) != VK_SUCCESS)
		THROW_ERROR("failed to create shader module!");
}

void Shader::clean() {
	if(!shader) return;
	vkDestroyShaderModule(device, shader, nullptr);
	shader = nullptr;
}

void Pipeline::init(const Device &device, const Shader &vertexShader, const Shader &fragmentShader) {
		// Create shader stages
		VkPipelineShaderStageCreateInfo stages[2];
		for(VkPipelineShaderStageCreateInfo &stageInfo : stages) {
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.pNext = nullptr;
			stageInfo.flags = 0u;
			stageInfo.pName = "main";
			stageInfo.pSpecializationInfo = nullptr; // to setup constants
		}
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module = vertexShader;
		stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].module = fragmentShader;
}

}