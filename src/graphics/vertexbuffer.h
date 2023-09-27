// Copyright (C) 2023, Coudert--Osmont Yoann
// SPDX-License-Identifier: AGPL-3.0-or-later
// See <https://www.gnu.org/licenses/>

#pragma once

#include "buffer.h"
#include "maths.h"

#include <array>

namespace gfx {

template<typename T> VkFormat typeFormat();
template<> constexpr VkFormat typeFormat<vec3>() { return VK_FORMAT_R64G64B64_SFLOAT; }
template<> constexpr VkFormat typeFormat<vec2>() { return VK_FORMAT_R64G64_SFLOAT; }
template<> constexpr VkFormat typeFormat<vec3f>() { return VK_FORMAT_R32G32B32_SFLOAT; }
template<> constexpr VkFormat typeFormat<vec2f>() { return VK_FORMAT_R32G32_SFLOAT; }

template<typename... T>
struct VertexDescription {
	constexpr static VkVertexInputBindingDescription getBindingDescription() {
		return VkVertexInputBindingDescription {
			.binding = 0u,
			.stride = (sizeof(T) + ...),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		};
	}

	constexpr static std::array<VkVertexInputAttributeDescription, sizeof...(T)> getAttributeDescription() {
		VkFormat formats[] { typeFormat<T>() ... };
		uint sizes[] { sizeof(T) ... };
		std::array<VkVertexInputAttributeDescription, sizeof...(T)> attributes;
		uint offset = 0u;
		for(uint i = 0; i < sizeof...(T); ++i) {
			attributes[i].location = i;
			attributes[i].binding = 0u;
			attributes[i].format = formats[i];
			attributes[i].offset = offset;
			offset += sizes[i];
		}
		return attributes;
	}
};

struct Vertex : public VertexDescription<vec3f, vec3f, vec2f> {
	vec3f pos;
	vec3f normal;
	vec2f uv;
};

class VertexBuffer : public Buffer {
public:
	VertexBuffer() = default;

	inline void init(const Device &device, VkDeviceSize size) {
		Buffer::init(device, size,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}
	inline void init(const Device &device, const Vertex *vertices, VkDeviceSize size) {
		init(device, size);
		const Buffer tmp = Buffer::createStagingBuffer(device, (void*) vertices, size);
		Buffer::copy(device, tmp, *this, size);
	}
};

class IndexBuffer : public Buffer {
public:
	IndexBuffer() = default;

	inline void init(const Device &device, VkDeviceSize size) {
		Buffer::init(device, size,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}
	inline void init(const Device &device, const uint32_t *indices, VkDeviceSize size) {
		init(device, size);
		const Buffer tmp = Buffer::createStagingBuffer(device, (void*) indices, size);
		Buffer::copy(device, tmp, *this, size);
	}

};

}