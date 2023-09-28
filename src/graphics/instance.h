#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace gfx {

struct Extensions {
	uint32_t count;
	const char** exts;
};

class Instance {
public:
	~Instance() { clean(); }

	void init(const char* name, const Extensions &requiredExtensions);
	void clean();

	inline operator VkInstance() const { return instance; }

	#ifndef NDEBUG
	const static char* validation_layer;
	#endif

private:
	VkInstance instance = nullptr;

	#ifndef NDEBUG
	VkDebugUtilsMessengerEXT debugMessenger = nullptr;
	#endif
};

template<typename T, typename... Ts>
struct __LastPointerArg {
	using type = typename __LastPointerArg<Ts...>::type;
};
template<typename T>
struct __LastPointerArg<T*> {
	using type = T;
};
template<typename... Ts>
using __LastPointerArgType = typename __LastPointerArg<Ts...>::type;

template<typename... Args0, typename... Args>
auto vkGetList(auto (&f)(Args0...), Args... args) {
	uint32_t count = 0u;
	f(args..., &count, nullptr);
	std::vector<__LastPointerArgType<Args0...>> list(count);
	f(args..., &count, list.data());
	return list;
}

template<typename... Args0, typename... Args>
uint32_t vkGetListSize(auto (&f)(Args0...), Args... args) {
	uint32_t count = 0; f(args..., &count, nullptr);
	return count;
}

}