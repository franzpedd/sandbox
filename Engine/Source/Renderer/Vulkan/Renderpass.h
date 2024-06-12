#pragma once

#include "Util/Memory.h"
#include <volk.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace Cosmos::Vulkan
{
	// forward declarations
	class Device;

	class Renderpass
	{
	public:

		struct Specification
		{
			const char* name = nullptr;
			VkRenderPass renderPass = VK_NULL_HANDLE;
			VkCommandPool commandPool = VK_NULL_HANDLE;
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

			VkSampleCountFlagBits msaa = VK_SAMPLE_COUNT_1_BIT;
			std::vector<VkCommandBuffer> commandBuffers = {};
			std::vector<VkFramebuffer> frameBuffers = {};
		};

	public:

		// constructor
		Renderpass(Shared<Device> device, const char* name, VkSampleCountFlagBits msaa);

		// destructor
		~Renderpass();

		// returns a reference to the renderpass-related objects
		inline Specification& GetSpecificationRef() { return mSpecification; }

	private:

		Shared<Device> mDevice;
		Specification mSpecification = {};
	};

	class RenderpassManager
	{
	public:

		// constructor
		RenderpassManager(Shared<Device> device);

		// destructor
		~RenderpassManager() = default;

		// returns a reference to the renderpasses
		inline std::unordered_map<std::string, Shared<Renderpass>>& GetRenderpassesRef() { return mRenderpasses; }

		// returns the main renderpass at the momment
		inline Shared<Renderpass> GetMainRenderpass() { return mMainRenderpass; }

	public:

		// sets the main renderpass
		bool SetMain(const char* nameid);

		// returns if VKCommandEntry exists
		bool Exists(const char* nameid);

		// inserts a new VKCommandEntry, returns false if already exists
		bool Insert(const char* nameid, VkSampleCountFlagBits msaa);

		// erases a given VKCommandEntry given it's nameid, returns false if doesn't exists
		bool Erase(const char* nameid);

	private:

		Shared<Device> mDevice;
		Shared<Renderpass> mMainRenderpass;
		std::unordered_map<std::string, Shared<Renderpass>> mRenderpasses;
	};
}