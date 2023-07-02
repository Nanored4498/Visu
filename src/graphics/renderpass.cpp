#include "renderpass.h"

#include "debug.h"

namespace gfx {

void RenderPass::init(const Device &device, const Swapchain &swapchain) {
	clean();

	// depth format
	/*
	depthFormat = device.findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
	if(depthFormat == VK_FORMAT_MAX_ENUM) throw std::runtime_error("failed to find a supported depth format!");
	*/

	// attachments
	std::vector<VkAttachmentDescription> attachments {
		{ // Color
			.flags = 0u,
			.format = swapchain.getFormat(),
			// .samples = msaaSamples,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		} /*, { // Depth
			.flags = 0u,
			.format = depthFormat,
			.samples = msaaSamples,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		}*/
	};
	/*
	if(msaaSamples != VK_SAMPLE_COUNT_1_BIT) {
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments.push_back({ // Resolve
			.flags = 0u,
			.format = device.getFormat().format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		});
	}
	*/
	const VkAttachmentReference colAttachmentRef {
		.attachment = 0u,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	/*
	const VkAttachmentReference depthAttachmentRef {
		.attachment = 1u,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};
	const VkAttachmentReference resolveAttachmentRef {
		.attachment = 2u,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	*/

	const VkSubpassDescription subpass {
		.flags = 0u,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0u,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1u,
		.pColorAttachments = &colAttachmentRef,
		// .pResolveAttachments = msaaSamples != VK_SAMPLE_COUNT_1_BIT ? &resolveAttachmentRef : nullptr,
		.pResolveAttachments = nullptr,
		// .pDepthStencilAttachment = &depthAttachmentRef,
		.pDepthStencilAttachment = nullptr,
		.preserveAttachmentCount = 0u,
		.pPreserveAttachments = nullptr
	};

	const VkSubpassDependency dependcy {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0u,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT /*| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT */,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT /*| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT */,
		.srcAccessMask = 0u,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT /*| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT*/,
		.dependencyFlags = 0u
	};

	const VkRenderPassCreateInfo passInfo {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.attachmentCount = (uint) attachments.size(),
		.pAttachments = attachments.data(),
		.subpassCount = 1u,
		.pSubpasses = &subpass,
		.dependencyCount = 1u,
		.pDependencies = &dependcy
	};

	if(vkCreateRenderPass(this->device = device, &passInfo, nullptr, &pass) != VK_SUCCESS)
		THROW_ERROR("failed to create render pass!");
	
	initFramebuffers(swapchain);
}

void RenderPass::initFramebuffers(const Swapchain &swapchain) {
	cleanFramebuffers();
	VkFramebufferCreateInfo framebufferInfo {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.renderPass = pass,
		.attachmentCount = 0u,
		.pAttachments = nullptr,
		.width = swapchain.getWidth(),
		.height = swapchain.getHeight(),
		.layers = 1u
	};
	framebuffers.resize(swapchain.size());
	for(std::size_t i = 0; i < framebuffers.size(); ++i) {
		std::vector<VkImageView> attachments {
			swapchain[i]
			// , depthImage.getView()
		};
		/*
		if(msaaSamples != VK_SAMPLE_COUNT_1_BIT) {
			attachments.push_back(attachments[0]);
			attachments[0] = msaaImage.getView();
		}
		*/
		framebufferInfo.attachmentCount = (uint32_t) attachments.size(),
		framebufferInfo.pAttachments = attachments.data();
		if(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
			THROW_ERROR("failed to create framebuffer!");
	}
}

void RenderPass::clean() {
	if(!pass) return;
	cleanFramebuffers();
	vkDestroyRenderPass(device, pass, nullptr);
	pass = nullptr;
}

void RenderPass::cleanFramebuffers() {
	for(VkFramebuffer framebuffer : framebuffers)
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	framebuffers.clear();
}

}