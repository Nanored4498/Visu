// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

#include "pipeline.h"

#include "debug.h"
#include "vertexbuffer.h"

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

void Pipeline::init(const Device &device, const Shader &vertexShader, const Shader &fragmentShader, const RenderPass &renderPass) {
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
		
	// Input
	const auto bindingDescription = Vertex::getBindingDescription();
	const auto attributeDescriptions = Vertex::getAttributeDescription();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.vertexBindingDescriptionCount = 1u,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = (uint) attributeDescriptions.size(),
		.pVertexAttributeDescriptions = attributeDescriptions.data()
	};
	VkPipelineInputAssemblyStateCreateInfo assemblyInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	// Viewport (We use a dynamic viewport so there is only nullptrs)
	const VkPipelineViewportStateCreateInfo viewportInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.viewportCount = 1u,
		.pViewports = nullptr,
		.scissorCount = 1u,
		.pScissors = nullptr
	};

	// Rasterization
	const VkPipelineRasterizationStateCreateInfo rasterInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.depthClampEnable = VK_FALSE, // Clamp fragments not in depth range instead of removing it
		.rasterizerDiscardEnable = VK_FALSE, // We want to rasterize
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.f,
		.depthBiasClamp = 0.f,
		.depthBiasSlopeFactor = 0.f,
		.lineWidth = 1.f // Require wideline feature to be < 1.f
	};

	// Multisampling
	const VkPipelineMultisampleStateCreateInfo multiSampleInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		// .rasterizationSamples = renderPass.getMsaaSamples(),
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.f,
		.pSampleMask = nullptr,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE
	};

	// Depth and stencil
	/*
	const VkPipelineDepthStencilStateCreateInfo depthStencilInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {},
		.minDepthBounds = 0.f,
		.maxDepthBounds = 1.f
	};
	*/

	// Color blending
	const VkPipelineColorBlendAttachmentState colorBlendAttachment {
		.blendEnable = VK_FALSE,
		// .blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
						| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
	const VkPipelineColorBlendStateCreateInfo colorBlendInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1u,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = { 0.f, 0.f, 0.f, 0.f }
	};

	// Dynamic state
	const VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	const VkPipelineDynamicStateCreateInfo dynamicInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.dynamicStateCount = std::size(dynamicStates),
		.pDynamicStates = dynamicStates
	};

	// Layout
	// const VkDescriptorSetLayout layouts[] { descriptorPool.layout };
	const VkPipelineLayoutCreateInfo layoutInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		// .setLayoutCount = std::size(layouts),
		// .pSetLayouts = layouts,
		.setLayoutCount = 0u,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 0u,
		.pPushConstantRanges = nullptr
	};

	if(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
		THROW_ERROR("failed to create pipeline layout!");

	// Pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.stageCount = std::size(stages), // shader stages
		.pStages = stages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &assemblyInfo,
		.pTessellationState = nullptr,
		.pViewportState = &viewportInfo,
		.pRasterizationState = &rasterInfo,
		.pMultisampleState = &multiSampleInfo,
		// .pDepthStencilState = &depthStencilInfo,
		.pDepthStencilState = nullptr,
		.pColorBlendState = &colorBlendInfo,
		.pDynamicState = &dynamicInfo,
		.layout = layout,
		.renderPass = renderPass,
		.subpass = 0u,
		.basePipelineHandle = VK_NULL_HANDLE, // No heritage
		.basePipelineIndex = -1
	};

	if(vkCreateGraphicsPipelines(this->device = device, VK_NULL_HANDLE, 1u, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
		THROW_ERROR("failed to create graphics pipeline!");
}

void Pipeline::clean() {
	if(!pipeline) return;
	vkDestroyPipelineLayout(device, layout, nullptr);
	vkDestroyPipeline(device, pipeline, nullptr);
	pipeline = nullptr;
}

}