#pragma once

#include "debug.h"
#include "renderpass.h"
#include "pipeline.h"
#include "vertexbuffer.h"
#include "sync.h"

namespace gfx {

class CommandBuffer {
public:
	CommandBuffer() {};

	inline operator VkCommandBuffer() const { return cmd; }

	CommandBuffer& begin() {
		constexpr const VkCommandBufferBeginInfo beginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0u,
			.pInheritanceInfo = nullptr // Not usefull here because the buffer is primary
		};
		if(vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) THROW_ERROR("failed to begin recording command buffer!");
		return *this;
	}

	CommandBuffer& beginOT() {
		constexpr const VkCommandBufferBeginInfo beginInfo {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr // Not usefull here because the buffer is primary
		};
		if(vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) THROW_ERROR("failed to begin recording one time command buffer!");
		return *this;
	}

	inline CommandBuffer& end() {
		if(vkEndCommandBuffer(cmd) != VK_SUCCESS) THROW_ERROR("failed to record command buffer!");
		return *this;
	}

	CommandBuffer& beginRenderPass(const RenderPass &renderPass, const Swapchain &swapchain, const std::size_t frame) {
		constexpr const VkClearValue clearValues[] = {
			{.color={.float32={0.f, 0.f, 0.f, 1.f}}},
			{.depthStencil={1.f, 0u}}
		};
		VkRenderPassBeginInfo renderPassInfo {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = renderPass,
			.framebuffer = renderPass.framebuffer(frame),
			.renderArea = {
				.offset = {0, 0},
				.extent = swapchain.getExtent()
			},
			.clearValueCount = std::size(clearValues),
			.pClearValues = clearValues
		};
		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		return *this;
	}

	inline CommandBuffer& endRenderPass() { vkCmdEndRenderPass(cmd); return *this; }

	CommandBuffer& setViewport(const VkExtent2D &extent) {
		const VkViewport viewport {
			.x = 0.f,
			.y = 0.f,
			.width = (float) extent.width,
			.height = (float) extent.height,
			.minDepth = 0.f,
			.maxDepth = 1.f
		};
		const VkRect2D scissor {
			.offset = { 0, 0 },
			.extent = extent
		};
		vkCmdSetViewport(cmd, 0u, 1u, &viewport);
		vkCmdSetScissor(cmd, 0u, 1u, &scissor);
		return *this;
	}

	inline CommandBuffer& bindPipeline(const Pipeline &pipeline) { vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline); return *this; }

	inline CommandBuffer& bindVertexBuffer(const VertexBuffer &vertexBuffer) {
		const VkBuffer buffer = vertexBuffer;
		const VkDeviceSize offset = 0u;
		vkCmdBindVertexBuffers(cmd, 0u, 1u, &buffer, &offset);
		return *this;
	}

	inline CommandBuffer& draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
		vkCmdDraw(cmd, vertexCount, instanceCount, firstVertex, firstInstance); return *this;
	}

	struct SubmitSync {
		Semaphore imageAvailable, renderFinished;
		Fence inFlight;
		~SubmitSync() { clean(); }
		void init(const Device &device) {
			imageAvailable.init(device);
			renderFinished.init(device);
			inFlight.init(device, true);
		}
		void clean() {
			imageAvailable.clean();
			renderFinished.clean();
			inFlight.clean();
		}
	};

	static void submit(CommandBuffer *cmds, uint32_t count, VkQueue queue, Semaphore &wait, Semaphore &signal, Fence &fence) {
		const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		const VkSubmitInfo submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = wait ? 1u : 0u,
			.pWaitSemaphores = &wait,
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = count,
			.pCommandBuffers = reinterpret_cast<VkCommandBuffer*>(cmds),
			.signalSemaphoreCount = signal ? 1u : 0u,
			.pSignalSemaphores = &signal
		};
		if(vkQueueSubmit(queue, 1u, &submitInfo, fence) != VK_SUCCESS)
			THROW_ERROR("failed to submit draw command buffer!");
	}

	inline void submit(VkQueue queue, Semaphore &wait, Semaphore &signal, Fence &fence) {
		submit(this, 1, queue, wait, signal, fence);
	}

	void submit(VkQueue queue) {
		const VkSubmitInfo submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0u,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1u,
			.pCommandBuffers = &cmd,
			.signalSemaphoreCount = 0u,
			.pSignalSemaphores = nullptr
		};
		if(vkQueueSubmit(queue, 1u, &submitInfo, nullptr) != VK_SUCCESS)
			THROW_ERROR("failed to submit draw command buffer!");
	}

private:
	VkCommandBuffer cmd;
};

class CommandBuffers {
public:
	// ~CommandBuffers() { clear(); }
	inline void init(const Device &device) { this->device = &device; }
	inline void resize(const std::size_t size, const bool primary=true) {
		const std::size_t s = cmds.size();
		if(size < s) device->freeCommandBuffers(cmds.data() + size, s-size);
		cmds.resize(size);
		if(s < size) device->allocCommandBuffers(cmds.data() + s, size-s, primary); 
	}
	inline void clear() { resize(0); }
	inline std::size_t size() const { return cmds.size(); }
	inline CommandBuffer& operator[](std::size_t n) { return cmds[n]; }
	inline auto begin() { return cmds.begin(); }
	inline auto end() { return cmds.end(); }

private:
	std::vector<CommandBuffer> cmds;
	const Device *device = nullptr;
};

}