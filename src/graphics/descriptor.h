// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

#pragma once

#include "buffer.h"

namespace gfx {

class DescriptorPool {
public:
	~DescriptorPool() { clean(); }

	void init(const Device &device, uint32_t size);
	void clean();

	inline void addUniformBuffer(VkShaderStageFlags shaderStage, VkDeviceSize size, uint32_t count = 1u) {
		addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, count, shaderStage, size);
	}

	inline const VkDescriptorSetLayout& getLayout() const { return layout; }

	inline Buffer& getBuffer() { return uniformBuffer; }
	inline VkDeviceSize getOffset(uint32_t set, uint32_t binding, uint32_t arrayElement = 0u) {
		return set * uniformBufferSize + bufferRanges[binding].offset + arrayElement * bufferRanges[binding].storage;
	}

	inline const VkDescriptorSet& operator[](std::size_t i) const { return sets[i]; }

private:
	VkDescriptorPool pool = nullptr;
	VkDescriptorSetLayout layout;

	VkDevice device;

	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkDescriptorSet> sets;
	Buffer uniformBuffer;

	struct BufferRange {
		VkDeviceSize offset, size, storage;
	};
	std::vector<BufferRange> bufferRanges;
	VkDeviceSize uniformBufferSize;

	inline void addBinding(VkDescriptorType type, uint32_t count, VkShaderStageFlags shaderStage, VkDeviceSize size) {
		//TODO: for image sampler look at the last parameter
		bindings.emplace_back((uint32_t) bindings.size(), type, count, shaderStage, nullptr);
		bufferRanges.emplace_back(0, size, 0);
	}
};

}