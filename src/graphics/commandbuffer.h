#pragma once

#include "debug.h"
#include "renderpass.h"
#include "pipeline.h"
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
			.clearValueCount = sizeof(clearValues) / sizeof(clearValues[0]),
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

	void submit(VkQueue queue, Semaphore &wait, Semaphore &signal, Fence &fence) {
		const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		const VkSubmitInfo submitInfo {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = wait ? 1u : 0u,
			.pWaitSemaphores = &wait,
			.pWaitDstStageMask = &waitStage,
			.commandBufferCount = 1u,
			.pCommandBuffers = &cmd,
			.signalSemaphoreCount = signal ? 1u : 0u,
			.pSignalSemaphores = &signal
		};
		fence.wait();
		fence.reset();
		if(vkQueueSubmit(queue, 1u, &submitInfo, fence) != VK_SUCCESS)
			THROW_ERROR("failed to submit draw command buffer!");
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

class CommandBufferAllocator : public std::allocator<CommandBuffer> {
public:
	using std::allocator<CommandBuffer>::allocator;
	CommandBufferAllocator(const Device &device, bool primary): std::allocator<CommandBuffer>(), device(device), primary(primary) {}
	CommandBuffer* allocate(std::size_t n) {
		CommandBuffer* p = std::allocator<CommandBuffer>::allocate(n);
		device.allocCommandBuffers(p, n, primary);
		return p;
	}
	void deallocate(CommandBuffer *p, std::size_t n) {
		device.freeCommandBuffers(p, n);
		std::allocator<CommandBuffer>::deallocate(p, n);
	}
	template<class U> struct rebind {
		using other = std::conditional_t<std::is_same_v<U, CommandBuffer>, CommandBufferAllocator, std::allocator<U>>;
	};

private:
	const Device &device;
	const bool primary;
};

class CommandBuffers : public std::vector<CommandBuffer, CommandBufferAllocator>  {
public:
	CommandBuffers(const Device &device, bool primary=true): std::vector<CommandBuffer, CommandBufferAllocator>(CommandBufferAllocator(device, primary)) {}
};

}