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

	inline void addUniformBuffer(VkShaderStageFlags shaderStage, VkDeviceSize size) {
		addBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, shaderStage, size);
	}

	inline const VkDescriptorSetLayout& getLayout() const { return layout; }

	inline void* getUniformMap(uint32_t ind, uint32_t binding) { return (char*) uniformMap + ind * uniformBufferSize + bufferRanges[binding].offset; }

	inline const VkDescriptorSet& operator[](std::size_t i) const { return sets[i]; }

private:
	VkDescriptorPool pool = nullptr;
	VkDescriptorSetLayout layout;

	VkDevice device;

	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkDescriptorSet> sets;
	Buffer uniformBuffer;
	void* uniformMap;

	struct BufferRange {
		VkDeviceSize offset, size;
	};
	std::vector<BufferRange> bufferRanges;
	VkDeviceSize uniformBufferSize;

	inline void addBinding(VkDescriptorType type, uint32_t count, VkShaderStageFlags shaderStage, VkDeviceSize size) {
		//TODO: for image sampler look at the last parameter
		bindings.emplace_back((uint32_t) bindings.size(), type, count, shaderStage, nullptr);
		bufferRanges.emplace_back(0, size);
	}
};

}