#if defined COSMOS_RENDERER_VULKAN

#include "Instance.h"
#include "Util/Assert.h"

#if defined (_WIN32)
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

namespace Cosmos::Vulkan
{
	Instance::Instance(const char* engineName, const char* appName, bool validations)
	{
		mEngineName = engineName;
		mAppName = appName;
		mValidations = validations;

		// we first initialize volk
		LOG_ASSERT(volkInitialize() == VK_SUCCESS, "Failed to initialize volk");

		VkApplicationInfo applicationInfo = {};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pNext = nullptr;
		applicationInfo.pApplicationName = appName;
		applicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		applicationInfo.pEngineName = engineName;
		applicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
		applicationInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);

		auto extensions = GetRequiredExtensions();

		VkDebugUtilsMessengerCreateInfoEXT debugUtilsCI = {};
		VkInstanceCreateInfo instanceCI = {};
		instanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCI.flags = 0;
#if defined PLATFORM_APPLE
		instanceCI.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
		instanceCI.pApplicationInfo = &applicationInfo;

		if (validations)
		{
			instanceCI.enabledLayerCount = (uint32_t)mValidationList.size();
			instanceCI.ppEnabledLayerNames = mValidationList.data();

			debugUtilsCI = {};
			debugUtilsCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugUtilsCI.pNext = nullptr;
			debugUtilsCI.flags = 0;
			debugUtilsCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugUtilsCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugUtilsCI.pfnUserCallback = DebugCallback;
			debugUtilsCI.pUserData = nullptr;

			instanceCI.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugUtilsCI;
		}

		else
		{
			instanceCI.pNext = nullptr;
			instanceCI.enabledLayerCount = 0;
			instanceCI.ppEnabledLayerNames = nullptr;
		}

		instanceCI.enabledExtensionCount = (uint32_t)extensions.size();
		instanceCI.ppEnabledExtensionNames = extensions.data();
		LOG_ASSERT(vkCreateInstance(&instanceCI, nullptr, &mInstance) == VK_SUCCESS, "Failed to create Vulkan Instance");

		// send instance to volk
		volkLoadInstance(mInstance);

		if (validations)
		{
			debugUtilsCI = {};
			debugUtilsCI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugUtilsCI.pNext = nullptr;
			debugUtilsCI.flags = 0;
			debugUtilsCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugUtilsCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugUtilsCI.pfnUserCallback = DebugCallback;
			debugUtilsCI.pUserData = nullptr;
			LOG_ASSERT(vkCreateDebugUtilsMessengerEXT(mInstance, &debugUtilsCI, nullptr, &mDebugMessenger) == VK_SUCCESS, "Failed to create Vulkan Debug Messenger");
		}
	}

	Instance::~Instance()
	{
		if (mValidations)
		{
			vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
		}

		vkDestroyInstance(mInstance, nullptr);
	}

	std::vector<const char*> Instance::GetRequiredExtensions()
	{
		std::vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME };
#if defined(_WIN32)
		extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
		extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(_DIRECT2DISPLAY)
		extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
		extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
		extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
		extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#endif

#if defined(VK_USE_PLATFORM_MACOS_MVK) && (VK_HEADER_VERSION >= 216)
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

		if (mValidations)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		}
		return extensions;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL Instance::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallback, void* pUserData)
	{
		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			LOG_ASSERT("[Error]: Validation Layer : % s", pCallback->pMessage);
			return VK_FALSE;
		}

		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			LOG_ASSERT("[Warn]: Validation Layer: %s", pCallback->pMessage);
			return VK_FALSE;
		}

		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		{
			LOG_ASSERT("[Info]: Validation Layer: %s", pCallback->pMessage);
			return VK_FALSE;
		}

		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		{
			LOG_ASSERT("[Trace]: Validation Layer: %s", pCallback->pMessage);
			return VK_FALSE;
		}

		return VK_FALSE;
	}
}

#endif