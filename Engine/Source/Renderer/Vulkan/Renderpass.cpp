#include "Renderpass.h"
#include "epch.h"
#if defined COSMOS_RENDERER_VULKAN

#include "Renderpass.h"

#include "Device.h"
#include "Util/Logger.h"

namespace Cosmos::Vulkan
{
	Renderpass::Renderpass(Shared<Device> device, const char* name, VkSampleCountFlagBits msaa)
		: mDevice(device)
	{
		mSpecification.name = name;
		mSpecification.msaa = msaa;
	}

	Renderpass::~Renderpass()
	{
		vkDeviceWaitIdle(mDevice->GetLogicalDevice());

		vkDestroyDescriptorPool(mDevice->GetLogicalDevice(), mSpecification.descriptorPool, nullptr);
		vkDestroyRenderPass(mDevice->GetLogicalDevice(), mSpecification.renderPass, nullptr);

		vkFreeCommandBuffers(mDevice->GetLogicalDevice(), mSpecification.commandPool, (uint32_t)mSpecification.commandBuffers.size(), mSpecification.commandBuffers.data());
		vkDestroyCommandPool(mDevice->GetLogicalDevice(), mSpecification.commandPool, nullptr);

		for (auto& framebuffer : mSpecification.frameBuffers)
			vkDestroyFramebuffer(mDevice->GetLogicalDevice(), framebuffer, nullptr);

		mSpecification.frameBuffers.clear();
	}

	RenderpassManager::RenderpassManager(Shared<Device> device)
		: mDevice(device)
	{
	}

	bool RenderpassManager::SetMain(const char* nameid)
	{
		auto it = mRenderpasses.find(nameid);

		if (it != mRenderpasses.end())
		{
			mMainRenderpass = mRenderpasses[nameid];
			return true;
		}

		return false;
	}

	bool RenderpassManager::Exists(const char* nameid)
	{
		return mRenderpasses.find(nameid) != mRenderpasses.end() ? true : false;
	}

	bool RenderpassManager::Insert(const char* nameid, VkSampleCountFlagBits msaa)
	{
		auto it = mRenderpasses.find(nameid);

		if (it != mRenderpasses.end())
		{
			COSMOS_LOG(Logger::Error, "A Renderpass with name %s already exists, could not create another", nameid);
			return false;
		}

		mRenderpasses[nameid] = CreateShared<Renderpass>(mDevice, nameid, msaa);
		return true;
	}

	bool RenderpassManager::Erase(const char* nameid)
	{
		auto it = mRenderpasses.find(nameid);

		if (it != mRenderpasses.end())
		{
			mRenderpasses.erase(nameid);
			return true;
		}

		COSMOS_LOG(Logger::Error, "No Renderpass with name %s exists", nameid);
		return false;
	}
}

#endif