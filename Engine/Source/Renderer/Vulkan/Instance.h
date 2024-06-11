#pragma once

#include "Platform/Window.h"
#include <volk.h>
#include <vector>

namespace Cosmos::Vulkan
{
	class Instance
	{
	public:

		// constructor
		Instance(Shared<Window> window, const char* engineName, const char* appName, bool validations = true);

		// destructor
		~Instance();

	public:

		// returns a reference to the vulkan instance
		inline VkInstance GetInstance() const { return mInstance; }

		// returns a reference to the vulkan debug utils messenger
		inline VkDebugUtilsMessengerEXT GetDebugger() { return mDebugMessenger; }

		// returns if validations are enabled
		inline bool GetValidations() const { return mValidations; }

		// returns listed validations
		inline const std::vector<const char*> GetValidationsList() const { return mValidationList; }

	public:

		// returns the required extensions by the vkinstance
		std::vector<const char*> GetRequiredExtensions();

		// internal errors callback
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallback, void* pUserData);

	private:

		Shared<Window> mWindow;
		const char* mAppName = nullptr;
		const char* mEngineName = nullptr;
		bool mValidations = true;
		const std::vector<const char*> mValidationList = { "VK_LAYER_KHRONOS_validation" };
		VkInstance mInstance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
	};
}