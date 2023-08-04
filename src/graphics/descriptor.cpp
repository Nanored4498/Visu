// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

#include "descriptor.h"

#include "debug.h"

#include <algorithm>

namespace gfx {

void DescriptorPool::init(const Device &device, uint32_t size) {
	clean();

	// Init layout
	VkDescriptorSetLayoutCreateInfo layoutInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.bindingCount = (uint32_t) bindings.size(),
		.pBindings = bindings.data()
	};
	if(vkCreateDescriptorSetLayout(this->device = device, &layoutInfo, nullptr, &layout) != VK_SUCCESS)
		THROW_ERROR("failed to create description set layout!");

	// Compute pool sizes
	std::vector<VkDescriptorPoolSize> poolSizes;
	for(const VkDescriptorSetLayoutBinding &binding : bindings) {
		auto it = std::ranges::find(poolSizes, binding.descriptorType, [](const VkDescriptorPoolSize &ps) { return ps.type; });
		if(it == poolSizes.end()) poolSizes.emplace_back(binding.descriptorType, binding.descriptorCount);
		else it->descriptorCount += binding.descriptorCount;
	}
	for(VkDescriptorPoolSize &ps : poolSizes) ps.descriptorCount *= size;

	// Create descriptor pool
	VkDescriptorPoolCreateInfo poolInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.maxSets = size,
		.poolSizeCount = (uint) poolSizes.size(),
		.pPoolSizes = poolSizes.data()
	};
	if(vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
		THROW_ERROR("failed to create descriptor pool!");

	// Create descriptor sets
	std::vector<VkDescriptorSetLayout> layouts(size, (VkDescriptorSetLayout) layout);
	VkDescriptorSetAllocateInfo allocInfo {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = pool,
		.descriptorSetCount = size,
		.pSetLayouts = layouts.data()
	};
	sets.resize(layouts.size());
	if(vkAllocateDescriptorSets(device, &allocInfo, sets.data()) != VK_SUCCESS)
		THROW_ERROR("failed to allocate descriptor sets!");

	// Create uniform buffer
	uniformBufferSize = 0;
	VkDeviceSize uniformAlignment = device.getProperties().limits.minUniformBufferOffsetAlignment;
	uint32_t uniformBufferCount = 0;
	for(std::size_t i = 0; i < bindings.size(); ++i)
		if(bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
			bufferRanges[i].offset = uniformBufferSize;
			bufferRanges[i].storage = bufferRanges[i].size;
			if(bufferRanges[i].storage % uniformAlignment)
				bufferRanges[i].storage += uniformAlignment - (bufferRanges[i].storage % uniformAlignment);
			uniformBufferSize += bindings[i].descriptorCount * bufferRanges[i].storage;
			uniformBufferCount += bindings[i].descriptorCount;
		}
	if(uniformBufferSize) {
		uniformBuffer.init(device, size * uniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				//TODO: instead of coherent use flush
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		uniformMap = uniformBuffer.mapMemory();
	}
	
	// Update descriptor sets
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	bufferInfos.reserve(size * uniformBufferCount);
	std::vector<VkWriteDescriptorSet> writes(size * bindings.size());
	for(uint32_t i = 0; i < size; ++i) {
		for(std::size_t j = 0; j < bindings.size(); ++j) {
			writes[i * bindings.size() + j] = VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = sets[i],
				.dstBinding = (uint32_t) j,
				.dstArrayElement = 0u,
				.descriptorCount = bindings[j].descriptorCount,
				.descriptorType = bindings[j].descriptorType,
				.pImageInfo = nullptr,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr
			};
			if(bindings[j].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
				writes[i * bindings.size() + j].pBufferInfo = bufferInfos.data() + bufferInfos.size();
				for(uint32_t k = 0; k < bindings[j].descriptorCount; ++k)
					bufferInfos.emplace_back(uniformBuffer,
						i * uniformBufferSize + bufferRanges[j].offset + k * bufferRanges[j].storage,
						bufferRanges[j].size);
			} else ASSERT(false);
		}
	}
	vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0u, nullptr);
}

void DescriptorPool::clean() {
	if(!pool) return;
	uniformBuffer.clean();
	vkDestroyDescriptorPool(device, pool, nullptr);
	vkDestroyDescriptorSetLayout(device, layout, nullptr);
	pool = nullptr;
}

}