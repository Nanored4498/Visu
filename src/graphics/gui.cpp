// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>#include "gui.h"

#include "gui.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace gfx {

void GUIRenderPass::init(const Device &device, const Swapchain &swapchain) {
	clean();

	const VkAttachmentDescription attachments[] {
		{ // Color
			.flags = 0u,
			.format = swapchain.getFormat(),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		}
	};
	const VkAttachmentReference colAttachmentRef {
		.attachment = 0u,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	const VkSubpassDescription subpass {
		.flags = 0u,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0u,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1u,
		.pColorAttachments = &colAttachmentRef,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = nullptr,
		.preserveAttachmentCount = 0u,
		.pPreserveAttachments = nullptr
	};
	const VkSubpassDependency dependcy {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0u,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_NONE,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0u
	};
	const VkRenderPassCreateInfo passInfo {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.attachmentCount = std::size(attachments),
		.pAttachments = attachments,
		.subpassCount = 1u,
		.pSubpasses = &subpass,
		.dependencyCount = 1u,
		.pDependencies = &dependcy
	};
	if(vkCreateRenderPass(this->device = device, &passInfo, nullptr, &pass) != VK_SUCCESS)
		THROW_ERROR("failed to create gui render pass!");

	initFramebuffers(swapchain);
};

void GUIRenderPass::initFramebuffers(const Swapchain &swapchain) {
	cleanFramebuffers();
	VkImageView attachment;
	const VkFramebufferCreateInfo framebufferInfo {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.renderPass = pass,
		.attachmentCount = 1u,
		.pAttachments = &attachment,
		.width = swapchain.getWidth(),
		.height = swapchain.getHeight(),
		.layers = 1u
	};
	framebuffers.resize(swapchain.size());
	for(std::size_t i = 0; i < framebuffers.size(); ++i) {
		attachment = swapchain[i];
		if(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
			THROW_ERROR("failed to create gui framebuffer!");
	}
}

void GUI::init(Instance &instance, const Device &device, const Swapchain &swapchain) {
	clean();

	// Descriptor pool
	const VkDescriptorPoolSize pool_sizes[] {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 128 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 128 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 128 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 128 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 128 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 128 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 128 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 128 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 128 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 128 }
	};
	VkDescriptorPoolCreateInfo pool_info {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = 256,
		.poolSizeCount = std::size(pool_sizes),
		.pPoolSizes = pool_sizes
	};
	vkCreateDescriptorPool(this->device = device, &pool_info, nullptr, &descriptorPool);

	// Render pass
	renderPass.init(device, swapchain);

	// cmdbuf
	cmdBufs.init(device);
	cmdBufs.resize(swapchain.size());

	// Context
	ImGui_ImplVulkan_InitInfo info {
		.Instance = instance,
		.PhysicalDevice = device.getGPU(),
		.Device = device,
		.QueueFamily = device.getQueueFamilies().graphicsId,
		.Queue = device.getGraphicsQueue(),
		.PipelineCache = nullptr,
		.DescriptorPool = descriptorPool,
		.Subpass = 0u,
		// TODO: As swapchain could be resized need to rethink...
		.MinImageCount = (uint32_t) swapchain.size(),
		.ImageCount = (uint32_t) swapchain.size(),
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.UseDynamicRendering = false,
		.ColorAttachmentFormat = VK_FORMAT_UNDEFINED,
		.Allocator = nullptr,
		.CheckVkResultFn = nullptr
	};
	ImGui_ImplVulkan_Init(&info, renderPass);

	//execute a gpu command to upload imgui font textures
	auto cmd = device.createCommandBuffer().beginOT();
	ImGui_ImplVulkan_CreateFontsTexture(cmd);
	cmd.end().submitOT(device, device.getGraphicsQueue());
	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void GUI::clean() {
	if(!descriptorPool) return;
	ImGui_ImplVulkan_Shutdown();
	cmdBufs.clear();
	renderPass.clean();
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
	descriptorPool = nullptr;
}

void GUI::update(const Swapchain &swapchain) {
	renderPass.initFramebuffers(swapchain);
	cmdBufs.resize(swapchain.size());
}

CommandBuffer& GUI::getCommand(const Swapchain &swapchain, const uint32_t i) {
	cmdBufs[i].beginOT().beginRenderPass(renderPass, swapchain, i);
  	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBufs[i]);
	return cmdBufs[i].endRenderPass().end();
}

}