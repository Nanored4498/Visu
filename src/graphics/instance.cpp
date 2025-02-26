#include "instance.h"

#include "debug.h"

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <vector>

namespace gfx {

#ifndef NDEBUG
const char* Instance::validation_layer = "VK_LAYER_KHRONOS_validation";

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData) {

	if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}
#endif

void Instance::init(const char* name, const Extensions &requiredExtensions) {
	clean();

	// App info
	const VkApplicationInfo appInfo {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = name,
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_0
	};

	// Required extensions
	std::vector<const char*> reqExtensions(requiredExtensions.exts, requiredExtensions.exts + requiredExtensions.count);
	const std::vector<VkExtensionProperties> extensions = vkGetList(vkEnumerateInstanceExtensionProperties, nullptr);
	const auto extensionAvailable = [&](const char *ext) {
		return std::ranges::find_if(extensions, [&](const VkExtensionProperties &e) { return !strcmp(e.extensionName, ext); }) != extensions.end();
	};
	for(const char* ext : reqExtensions) if(!extensionAvailable(ext))
		THROW_ERROR(std::string("The required extension ") + ext + " is not available...");
	if(extensionAvailable(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
		reqExtensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
		if(extensionAvailable(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
			reqExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	#endif
	#ifndef NDEBUG
		const bool debug_utils = extensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		if(debug_utils) reqExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	#endif

	// Instance info
	VkInstanceCreateInfo insInfo {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0u,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = (uint32_t) reqExtensions.size(),
		.ppEnabledExtensionNames = reqExtensions.data()
	};
	#ifdef __APPLE__
		insInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	#endif

	// Validation Layer
	#ifndef NDEBUG
		const VkDebugUtilsMessengerCreateInfoEXT debugInfo {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.pNext = nullptr,
			.flags = 0u,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
									| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
									| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
								| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
								| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debugCallback,
			.pUserData = nullptr
		};
		const std::vector<VkLayerProperties> layers = vkGetList(vkEnumerateInstanceLayerProperties);
		const bool use_validation_layer = std::ranges::find_if(layers, [&](const VkLayerProperties &layer) {
			return !strcmp(layer.layerName, validation_layer);
		}) != layers.end();
		if(use_validation_layer) {
			insInfo.pNext = &debugInfo;
			insInfo.enabledLayerCount = 1u;
			insInfo.ppEnabledLayerNames = &validation_layer;
		} else {
			DEBUG_MSG("WARNING:", validation_layer, "not available...");
		}
	#endif

	// Instance creation !!!!!!!
	if(vkCreateInstance(&insInfo, nullptr, &instance) != VK_SUCCESS)
		THROW_ERROR("failed to create instance!");
	
	// Debug Callback
	#ifndef NDEBUG
		if(debug_utils && use_validation_layer) {
			const auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
			if(func == nullptr || func(instance, &debugInfo, nullptr, &debugMessenger) != VK_SUCCESS)
				DEBUG_MSG("WARNING: failed to create debug callback. Using default one instead...");
		}
	#endif
}

void Instance::clean() {
	if(!instance) return;
	#ifndef NDEBUG
		if(debugMessenger) {
			const auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if(func != nullptr) func(instance, debugMessenger, nullptr);
			debugMessenger = nullptr;
		}
	#endif
	vkDestroyInstance(instance, nullptr);
	instance = nullptr;
}

}