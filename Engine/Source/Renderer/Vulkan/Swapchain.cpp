#include "epch.h"
#include "Swapchain.h"

#include "Util/Logger.h"

#include <algorithm>
#include <array>

namespace Cosmos::Vulkan
{
	Swapchain::Swapchain(Shared<Window> window, Shared<Device> device, Shared<RenderpassManager> renderpassManager, uint32_t maxFrames)
		: mWindow(window), mDevice(device), mRenderpassManager(renderpassManager), mMaxFrames(maxFrames)
	{
		mRenderpassManager->Insert("Swapchain", mDevice->GetMSAA());

		CreateSwapchain();
		CreateImageViews();

		CreateRenderPass();
		CreateFramebuffers();

		CreateCommandPool();
		CreateCommandBuffers();
	}

	Swapchain::~Swapchain()
	{
		vkDestroyImageView(mDevice->GetLogicalDevice(), mDepthView, nullptr);
		vmaDestroyImage(mDevice->GetAllocator(), mDepthImage, mDepthMemory);

		vkDestroyImageView(mDevice->GetLogicalDevice(), mColorView, nullptr);
		vmaDestroyImage(mDevice->GetAllocator(), mColorImage, mColorMemory);

		for (auto imageView : mImageViews)
		{
			vkDestroyImageView(mDevice->GetLogicalDevice(), imageView, nullptr);
		}

		mImageViews.clear();

		vkDestroySwapchainKHR(mDevice->GetLogicalDevice(), mSwapchain, nullptr);
	}

	void Swapchain::CreateSwapchain()
	{
		Details details = QueryDetails();

		// query all important details of the swapchain
		mSurfaceFormat = ChooseSurfaceFormat(details.formats);
		mPresentMode = ChoosePresentMode(details.presentModes);
		mExtent = ChooseExtent(details.capabilities);

		mImageCount = details.capabilities.minImageCount + 1;
		if (details.capabilities.maxImageCount > 0 && mImageCount > details.capabilities.maxImageCount) mImageCount = details.capabilities.maxImageCount;

		// create the swapchain
		Device::QueueFamilyIndices indices = mDevice->FindQueueFamilies(mDevice->GetPhysicalDevice(), mDevice->GetSurface());
		uint32_t queueFamilyIndices[] = { indices.graphics.value(), indices.present.value() };

		VkSwapchainCreateInfoKHR swapchainCI = {};
		swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCI.pNext = nullptr;
		swapchainCI.flags = 0;
		swapchainCI.surface = mDevice->GetSurface();
		swapchainCI.minImageCount = mImageCount;
		swapchainCI.imageFormat = mSurfaceFormat.format;
		swapchainCI.imageColorSpace = mSurfaceFormat.colorSpace;
		swapchainCI.imageExtent = mExtent;
		swapchainCI.imageArrayLayers = 1;
		swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // for copying the images, allowing viewports

		if (indices.graphics != indices.present)
		{
			swapchainCI.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainCI.queueFamilyIndexCount = 2;
			swapchainCI.pQueueFamilyIndices = queueFamilyIndices;
		}

		else
		{
			swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		swapchainCI.preTransform = details.capabilities.currentTransform;
		swapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCI.presentMode = mPresentMode;
		swapchainCI.clipped = VK_TRUE;

		COSMOS_ASSERT(vkCreateSwapchainKHR(mDevice->GetLogicalDevice(), &swapchainCI, nullptr, &mSwapchain) == VK_SUCCESS, "Failed to create the Swapchain");

		// get the images in the swapchain
		vkGetSwapchainImagesKHR(mDevice->GetLogicalDevice(), mSwapchain, &mImageCount, nullptr);
		mImages.resize(mImageCount);
		vkGetSwapchainImagesKHR(mDevice->GetLogicalDevice(), mSwapchain, &mImageCount, mImages.data());
	}

	void Swapchain::CreateImageViews()
	{
		mImageViews.resize(mImages.size());

		for (size_t i = 0; i < mImages.size(); i++)
		{
			mImageViews[i] = mDevice->CreateImageView(mImages[i], mSurfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	void Swapchain::CreateRenderPass()
	{
		// attachments descriptions
		std::array<VkAttachmentDescription, 3> attachments = {};

		// color
		attachments[0].format = mSurfaceFormat.format;
		attachments[0].samples = mDevice->GetMSAA();
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// depth
		attachments[1].format = mDevice->FindSuitableDepthFormat();
		attachments[1].samples = mDevice->GetMSAA();
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// resolve
		attachments[2].format = mSurfaceFormat.format;
		attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		// finalLayout should not be VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as ui is a post render pass that will present

		// attachments references
		std::array<VkAttachmentReference, 3> references = {};

		references[0].attachment = 0;
		references[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		references[1].attachment = 1;
		references[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		references[2].attachment = 2;
		references[2].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// subpass
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &references[0];
		subpass.pDepthStencilAttachment = &references[1];
		subpass.pResolveAttachments = &references[2];

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassCI = {};
		renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCI.attachmentCount = (uint32_t)attachments.size();
		renderPassCI.pAttachments = attachments.data();
		renderPassCI.subpassCount = 1;
		renderPassCI.pSubpasses = &subpass;
		renderPassCI.dependencyCount = 1;
		renderPassCI.pDependencies = &dependency;

		auto& spec = mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef();
		COSMOS_ASSERT(vkCreateRenderPass(mDevice->GetLogicalDevice(), &renderPassCI, nullptr, &spec.renderPass) == VK_SUCCESS, "Failed to create render pass");
	}

	void Swapchain::CreateFramebuffers()
	{
		// create frame color resources
		VkFormat format = mSurfaceFormat.format;

		mDevice->CreateImage
		(
			mExtent.width,
			mExtent.height,
			1,
			1,
			mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef().msaa,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			mColorImage,
			mColorMemory
		);

		mColorView = mDevice->CreateImageView(mColorImage, format, VK_IMAGE_ASPECT_COLOR_BIT);

		// create frame depth resources
		format = mDevice->FindSuitableDepthFormat();

		mDevice->CreateImage
		(
			mExtent.width,
			mExtent.height,
			1,
			1,
			mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef().msaa,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			mDepthImage,
			mDepthMemory
		);

		mDepthView = mDevice->CreateImageView(mDepthImage, format, VK_IMAGE_ASPECT_DEPTH_BIT);

		// create frame buffers
		auto& spec = mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef();
		spec.frameBuffers.resize(mImageViews.size());

		for (size_t i = 0; i < mImageViews.size(); i++)
		{
			std::array<VkImageView, 3> attachments = { mColorView, mDepthView, mImageViews[i] };

			VkFramebufferCreateInfo framebufferCI = {};
			framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCI.renderPass = spec.renderPass;
			framebufferCI.attachmentCount = (uint32_t)attachments.size();
			framebufferCI.pAttachments = attachments.data();
			framebufferCI.width = mExtent.width;
			framebufferCI.height = mExtent.height;
			framebufferCI.layers = 1;
			COSMOS_ASSERT(vkCreateFramebuffer(mDevice->GetLogicalDevice(), &framebufferCI, nullptr, &spec.frameBuffers[i]) == VK_SUCCESS, "Failed to create framebuffer");
		}
	}

	void Swapchain::Cleanup()
	{
		vkDestroyImageView(mDevice->GetLogicalDevice(), mDepthView, nullptr);
		vmaDestroyImage(mDevice->GetAllocator(), mDepthImage, mDepthMemory);

		vkDestroyImageView(mDevice->GetLogicalDevice(), mColorView, nullptr);
		vmaDestroyImage(mDevice->GetAllocator(), mColorImage, mColorMemory);

		auto spec = mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef();
		for (uint32_t i = 0; i < spec.frameBuffers.size(); i++)
		{
			vkDestroyFramebuffer(mDevice->GetLogicalDevice(), spec.frameBuffers[i], nullptr);
		}

		for (auto imageView : mImageViews)
		{
			vkDestroyImageView(mDevice->GetLogicalDevice(), imageView, nullptr);
		}

		vkDestroySwapchainKHR(mDevice->GetLogicalDevice(), mSwapchain, nullptr);
	}

	void Swapchain::Recreate()
	{
		mWindow->Recreate();

		vkDeviceWaitIdle(mDevice->GetLogicalDevice());

		Cleanup();

		CreateSwapchain();
		CreateImageViews();
		CreateFramebuffers();
	}

	Swapchain::Details Swapchain::QueryDetails()
	{
		Details details = {};
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mDevice->GetPhysicalDevice(), mDevice->GetSurface(), &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice->GetPhysicalDevice(), mDevice->GetSurface(), &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice->GetPhysicalDevice(), mDevice->GetSurface(), &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice->GetPhysicalDevice(), mDevice->GetSurface(), &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice->GetPhysicalDevice(), mDevice->GetSurface(), &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	void Swapchain::CreateCommandPool()
	{
		Device::QueueFamilyIndices indices = mDevice->FindQueueFamilies(mDevice->GetPhysicalDevice(), mDevice->GetSurface());
		auto& spec = mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef();
		
		VkCommandPoolCreateInfo cmdPoolInfo = {};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.queueFamilyIndex = indices.graphics.value();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		COSMOS_ASSERT(vkCreateCommandPool(mDevice->GetLogicalDevice(), &cmdPoolInfo, nullptr, &spec.commandPool) == VK_SUCCESS, "Failed to create command pool");
	}

	void Swapchain::CreateCommandBuffers()
	{
		auto& spec = mRenderpassManager->GetRenderpassesRef()["Swapchain"]->GetSpecificationRef();
		spec.commandBuffers.resize(mMaxFrames);

		VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.commandPool = spec.commandPool;
		cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocInfo.commandBufferCount = (uint32_t)spec.commandBuffers.size();
		COSMOS_ASSERT(vkAllocateCommandBuffers(mDevice->GetLogicalDevice(), &cmdBufferAllocInfo, spec.commandBuffers.data()) == VK_SUCCESS, "Failed to create command buffers");
	}

	VkSurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		COSMOS_LOG(Logger::Todo, "Decide on better format (VK_FORMAT_R8G8B8A8_SRGB) ?");

		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR Swapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vsync)
	{
		for (const auto& availablePresentMode : availablePresentModes)
		{
			// triple-buffer
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		// vsync
		if (vsync) return VK_PRESENT_MODE_FIFO_KHR;

		// render as is
		return VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

	VkExtent2D Swapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) return capabilities.currentExtent;

		int32_t width, height;
		mWindow->GetFrameBufferSize(&width, &height);

		VkExtent2D actualExtent = { (uint32_t)width, (uint32_t)height };
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}