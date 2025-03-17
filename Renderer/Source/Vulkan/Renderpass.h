#pragma once

#include <volk.h>
#include <memory>
#include <vector>

namespace Cosmos::Vulkan
{
	// forward declaration
	class Device;

	class Renderpass
	{
	public:

		// constructor
		Renderpass(std::shared_ptr<Device> device, const char* name, VkSampleCountFlagBits msaa);

		// destructor
		~Renderpass();

		// returns the ptr to the renderpass
		inline VkRenderPass GetRenderpass() const { return mRenderPass; }

		// returns the ptr to the command pool
		inline VkCommandPool GetCommandPool() const { return mCommandPool; }

		// returns the ptr to the descriptor pool
		inline VkDescriptorPool GetDescriptorPool() const { return mDescriptorPool;  }

		// returns a reference to the vector of command buffers
		inline std::vector<VkCommandBuffer>& GetCommandfuffersRef() { return mCommandfuffers; }

		// returns a referencec to the vector of frame buffers
		inline std::vector<VkFramebuffer>& GetFramebuffersRef() { return mFramebuffers; }

	private:

		std::shared_ptr<Device> mDevice;
		const char* mName = nullptr;
		VkSampleCountFlagBits mMSAA = VK_SAMPLE_COUNT_1_BIT;

		VkRenderPass mRenderPass = VK_NULL_HANDLE;
		VkCommandPool mCommandPool = VK_NULL_HANDLE;
		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;

		std::vector<VkCommandBuffer> mCommandfuffers = {};
		std::vector<VkFramebuffer> mFramebuffers = {};
	};
}