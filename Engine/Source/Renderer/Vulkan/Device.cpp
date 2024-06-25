#include "epch.h"
#if defined COSMOS_RENDERER_VULKAN

#include "Device.h"

#include "Instance.h"
#include "Platform/Window.h"
#include "Util/Logger.h"

#include <set>

namespace Cosmos::Vulkan
{
	Device::Device(Shared<Window> window, Shared<Instance> instance)
		: mWindow(window), mInstance(instance)
	{
		mWindow->CreateSurface(instance->GetInstance(), &(void*&)mSurface);
		SelectPhysicalDevice();

		// get physical device properties
		vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mMemoryProperties);
		vkGetPhysicalDeviceProperties(mPhysicalDevice, &mProperties);
		vkGetPhysicalDeviceFeatures(mPhysicalDevice, &mFeatures);

		CreateLogicalDevice();
		CreateAllocator();

		COSMOS_LOG(Logger::Todo, "Put MSAA on settings.ini");
		mMSAACount = GetMaxUsableSamples();
	}

	Device::~Device()
	{
		vmaDestroyAllocator(mAllocator);

		vkDestroyDevice(mDevice, nullptr);
		vkDestroySurfaceKHR(mInstance->GetInstance(), mSurface, nullptr);
	}

	Device::QueueFamilyIndices Device::FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphics = i;
			if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) indices.compute = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

			if (presentSupport) indices.present = i;
			if (indices.IsComplete()) break;
		}

		if (!indices.compute.has_value()) COSMOS_LOG(Logger::Severity::Warn, "A compute queue was not found");
		return indices;
	}

	VkSampleCountFlagBits Device::GetMaxUsableSamples()
	{
		COSMOS_LOG(Logger::Severity::Trace, "TODO: Move MSAA samples to settings.ini");

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

		VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	uint32_t Device::GetMemoryType(uint32_t bits, VkMemoryPropertyFlags properties, VkBool32* found)
	{
		for (uint32_t i = 0; i < mMemoryProperties.memoryTypeCount; i++)
		{
			if ((bits & 1) == 1)
			{
				if ((mMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					if (found)
					{
						*found = true;
					}

					return i;
				}
			}

			bits >>= 1;
		}

		if (found)
		{
			*found = false;
			return 0;
		}

		COSMOS_ASSERT(false, "Device: Could not find a matching memory type");
		return 0;
	}

	VkResult Device::CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceSize size, VkBuffer* buffer, VmaAllocation* memory, void* data)
	{
		// specify buffer
		VmaAllocationCreateInfo allocCI = {};
		allocCI.usage = VMA_MEMORY_USAGE_AUTO;
		allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VmaAllocationInfo allocInfo = {};

		VkBufferCreateInfo bufferCI = {};
		bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCI.size = size;
		bufferCI.usage = usage;
		bufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		COSMOS_ASSERT(vmaCreateBuffer(mAllocator, &bufferCI, &allocCI, buffer, memory, &allocInfo) == VK_SUCCESS, "Failed to create buffer");

		VkMemoryPropertyFlags memPropFlags;
		vmaGetAllocationMemoryProperties(mAllocator, *memory, &memPropFlags);

		if (memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && data != nullptr)
		{
			void* mapped = nullptr;
			COSMOS_ASSERT(vmaMapMemory(mAllocator, *memory, &mapped) == VK_SUCCESS, "Failed to map memory");
			memcpy(mapped, data, size);

			if ((properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				vmaFlushAllocation(mAllocator, *memory, 0, size);
			}
			
			vmaUnmapMemory(mAllocator, *memory);
		}

		return VK_SUCCESS;
	}

	VkCommandBuffer Device::CreateCommandBuffer(VkCommandPool cmdPool, VkCommandBufferLevel level, bool begin)
	{
		VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.pNext = nullptr;
		cmdBufferAllocInfo.commandBufferCount = 1;
		cmdBufferAllocInfo.commandPool = cmdPool;
		cmdBufferAllocInfo.level = level;

		VkCommandBuffer cmdBuffer;
		COSMOS_ASSERT(vkAllocateCommandBuffers(mDevice, &cmdBufferAllocInfo, &cmdBuffer) == VK_SUCCESS, "Failed to allocate command buffers");

		if (begin)
		{
			VkCommandBufferBeginInfo cmdBufferBI = {};
			cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			cmdBufferBI.pNext = nullptr;
			cmdBufferBI.flags = 0;
			COSMOS_ASSERT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBI) == VK_SUCCESS, "Failed to initialize command buffer");
		}

		return cmdBuffer;
	}

	void Device::BeginCommandBuffer(VkCommandBuffer cmdBuffer)
	{
		VkCommandBufferBeginInfo cmdBufferBI = {};
		cmdBufferBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBI.pNext = nullptr;
		cmdBufferBI.flags = 0;
		COSMOS_ASSERT(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBI) == VK_SUCCESS, "Failed to initialize command buffer");
	}

	void Device::EndCommandBuffer(VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, bool free)
	{
		COSMOS_ASSERT(vkEndCommandBuffer(cmdBuffer) == VK_SUCCESS, "Failed to end the recording of the command buffer");

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;

		VkFenceCreateInfo fenceCI = {};
		fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCI.pNext = nullptr;
		fenceCI.flags = 0;

		VkFence fence;
		COSMOS_ASSERT(vkCreateFence(mDevice, &fenceCI, nullptr, &fence) == VK_SUCCESS, "Failed to create fence for command buffer submission");
		COSMOS_ASSERT(vkQueueSubmit(queue, 1, &submitInfo, fence) == VK_SUCCESS, "Failed to submit command buffer");
		COSMOS_ASSERT(vkWaitForFences(mDevice, 1, &fence, VK_TRUE, 100000000000) == VK_SUCCESS, "Failed to wait for fences");

		vkDestroyFence(mDevice, fence, nullptr);

		if (free) vkFreeCommandBuffers(mDevice, cmdPool, 1, &cmdBuffer);
	}

	VkCommandBuffer Device::BeginSingleTimeCommand(VkCommandPool commandPool)
	{
		VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
		cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cmdBufferAllocInfo.commandPool = commandPool;
		cmdBufferAllocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(mDevice, &cmdBufferAllocInfo, &commandBuffer);

		VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &cmdBufferBeginInfo);
		return commandBuffer;
	}

	void Device::EndSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(mGraphicsQueue);

		vkFreeCommandBuffers(mDevice, commandPool, 1, &commandBuffer);
	}

	VkFormat Device::FindSuitableFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		VkFormat resFormat = VK_FORMAT_UNDEFINED;

		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) resFormat = format;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) resFormat = format;
		}

		COSMOS_ASSERT(resFormat != VK_FORMAT_UNDEFINED, "Failed to find suitable format");
		return resFormat;
	}

	VkFormat Device::FindSuitableDepthFormat()
	{
		const std::vector<VkFormat>& candidates =
		{
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};

		return FindSuitableFormat(candidates, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}

	void Device::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VmaAllocation& allocation, VkImageCreateFlags flags, VmaAllocationCreateFlags vmaFlags)
	{
		// specify image
		VkImageCreateInfo imageCI = {};
		imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCI.pNext = nullptr;
		imageCI.flags = flags;
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.extent.width = width;
		imageCI.extent.height = height;
		imageCI.extent.depth = 1;
		imageCI.mipLevels = mipLevels;
		imageCI.arrayLayers = arrayLayers;
		imageCI.format = format;
		imageCI.tiling = tiling;
		imageCI.usage = usage;
		imageCI.samples = samples;
		imageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// specify vma info
		VmaAllocationCreateInfo vmaCI = {};
		vmaCI.usage = VMA_MEMORY_USAGE_AUTO;
		vmaCI.flags = vmaFlags;

		// create image
		COSMOS_ASSERT(vmaCreateImage(mAllocator, &imageCI, &vmaCI, &image, &allocation, nullptr) == VK_SUCCESS, "Failed to create image");
	}

	VkImageView Device::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevel, uint32_t layerCount, VkImageViewType viewType)
	{
		VkImageView imageView = VK_NULL_HANDLE;
		VkImageViewCreateInfo imageViewCI = {};
		imageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCI.image = image;
		imageViewCI.viewType = viewType;
		imageViewCI.format = format;
		imageViewCI.subresourceRange.aspectMask = aspect;
		imageViewCI.subresourceRange.baseMipLevel = 0;
		imageViewCI.subresourceRange.levelCount = mipLevel;
		imageViewCI.subresourceRange.baseArrayLayer = 0;
		imageViewCI.subresourceRange.layerCount = layerCount;
		COSMOS_ASSERT(vkCreateImageView(mDevice, &imageViewCI, nullptr, &imageView) == VK_SUCCESS, "Failed to create image view");

		return imageView;
	}

	VkSampler Device::CreateSampler(VkFilter min, VkFilter mag, VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w, float mipLevels)
	{
		VkSampler sampler = VK_NULL_HANDLE;
		VkSamplerCreateInfo samplerCI = {};
		samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCI.magFilter = mag;
		samplerCI.minFilter = min;
		samplerCI.addressModeU = u;
		samplerCI.addressModeV = v;
		samplerCI.addressModeW = w;
		samplerCI.anisotropyEnable = VK_TRUE;
		samplerCI.maxAnisotropy = mProperties.limits.maxSamplerAnisotropy;
		samplerCI.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerCI.unnormalizedCoordinates = VK_FALSE;
		samplerCI.compareEnable = VK_FALSE;
		samplerCI.maxLod = mipLevels;
		samplerCI.minLod = 0.0f;
		samplerCI.mipLodBias = 0.0f;
		samplerCI.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		COSMOS_ASSERT(vkCreateSampler(mDevice, &samplerCI, nullptr, &sampler) == VK_SUCCESS, "Failed to create sampler");

		return sampler;
	}

	void Device::TransitionImageLayout(VkCommandPool& cmdPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, uint32_t layerCount)
	{
		VkCommandBuffer cmdBuffer = BeginSingleTimeCommand(cmdPool);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = layerCount;

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}

		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		else
		{
			COSMOS_ASSERT(false, "Invalid layout transition");
		}

		vkCmdPipelineBarrier(cmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		EndSingleTimeCommand(cmdPool, cmdBuffer);
	}

	void Device::InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subresourceRange)
	{
		VkImageMemoryBarrier imageMemoryBarrier = {};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.pNext = 0;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.srcAccessMask = srcAccessMask;
		imageMemoryBarrier.dstAccessMask = dstAccessMask;
		imageMemoryBarrier.oldLayout = oldImageLayout;
		imageMemoryBarrier.newLayout = newImageLayout;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	}

	void Device::SelectPhysicalDevice()
	{
		uint32_t gpuCount = 0;
		vkEnumeratePhysicalDevices(mInstance->GetInstance(), &gpuCount, nullptr);

		std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
		vkEnumeratePhysicalDevices(mInstance->GetInstance(), &gpuCount, physicalDevices.data());

		COSMOS_ASSERT(gpuCount > 0, "Could not find any GPU");

		// choose the only gpu available if there's only one available
		if (gpuCount == 1)
		{
			mPhysicalDevice = physicalDevices[0];
			return;
		}

		// selects first discrete gpu available if computer has multiple gpus, this is not ideal for a distro app
		COSMOS_LOG(Logger::Severity::Warn, "Choosing first discrete GPU available as it's a multiple GPU computer");

		for (VkPhysicalDevice& device : physicalDevices)
		{
			VkPhysicalDeviceProperties properties = {};
			vkGetPhysicalDeviceProperties(device, &properties);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				mPhysicalDevice = device;
				return;
			}
		}
	}

	void Device::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(mPhysicalDevice, mSurface);

		float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> deviceQueueCIs;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphics.value(), indices.present.value(), indices.compute.value() };

		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo deviceQueueCI = {};
			deviceQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			deviceQueueCI.pNext = nullptr;
			deviceQueueCI.flags = 0;
			deviceQueueCI.queueCount = 1;
			deviceQueueCI.queueFamilyIndex = queueFamily;
			deviceQueueCI.pQueuePriorities = &queuePriority;
			deviceQueueCIs.push_back(deviceQueueCI);
		}

		std::vector<const char*> extensions = {};
		extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#if defined PLATFORM_APPLE
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

		VkDeviceCreateInfo deviceCI = {};
		deviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCI.pNext = nullptr;
		deviceCI.flags = 0;
		deviceCI.queueCreateInfoCount = (uint32_t)deviceQueueCIs.size();
		deviceCI.pQueueCreateInfos = deviceQueueCIs.data();
		deviceCI.pEnabledFeatures = &mFeatures;
		deviceCI.enabledExtensionCount = (uint32_t)extensions.size();
		deviceCI.ppEnabledExtensionNames = extensions.data();

		std::vector<const char*> validations = mInstance->GetValidationsList();
		if (mInstance->GetValidations())
		{
			deviceCI.enabledLayerCount = (uint32_t)validations.size();
			deviceCI.ppEnabledLayerNames = validations.data();
		}

		else
		{
			deviceCI.enabledLayerCount = 0;
			deviceCI.ppEnabledLayerNames = nullptr;
		}

		COSMOS_ASSERT(vkCreateDevice(mPhysicalDevice, &deviceCI, nullptr, &mDevice) == VK_SUCCESS, "Failed to create Vulkan Device");

		vkGetDeviceQueue(mDevice, indices.graphics.value(), 0, &mGraphicsQueue);
		vkGetDeviceQueue(mDevice, indices.present.value(), 0, &mPresentQueue);
		vkGetDeviceQueue(mDevice, indices.compute.value(), 0, &mComputeQueue);
	}

	void Device::CreateAllocator()
	{
		VmaVulkanFunctions functions = {};
		functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
		functions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
		functions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
		functions.vkAllocateMemory = vkAllocateMemory;
		functions.vkFreeMemory = vkFreeMemory;
		functions.vkMapMemory = vkMapMemory;
		functions.vkUnmapMemory = vkUnmapMemory;
		functions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
		functions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
		functions.vkBindBufferMemory = vkBindBufferMemory;
		functions.vkBindImageMemory = vkBindImageMemory;
		functions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
		functions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
		functions.vkCreateBuffer = vkCreateBuffer;
		functions.vkDestroyBuffer = vkDestroyBuffer;
		functions.vkCreateImage = vkCreateImage;
		functions.vkDestroyImage = vkDestroyImage;
		functions.vkCmdCopyBuffer = vkCmdCopyBuffer;
		// these can be aquired from KHR at the end if available, since we're using volk it'll have their extensions loaded, but i'll fetch normal functions for now
		functions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
		functions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
		functions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
		functions.vkBindImageMemory2KHR = vkBindImageMemory2;
		functions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
		// these are vulkan 1.3 related
		//functions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
		//functions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

		VmaAllocatorCreateInfo ci = {};
		ci.flags = 0;
		ci.physicalDevice = mPhysicalDevice;
		ci.device = mDevice;
		ci.preferredLargeHeapBlockSize = 0;
		ci.pAllocationCallbacks = nullptr;
		ci.pDeviceMemoryCallbacks = nullptr;
		ci.pHeapSizeLimit = nullptr;
		ci.pVulkanFunctions = &functions;
		ci.instance = mInstance->GetInstance();
		ci.vulkanApiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);
		ci.pTypeExternalMemoryHandleTypes = nullptr;
		COSMOS_ASSERT(vmaCreateAllocator(&ci, &mAllocator) == VK_SUCCESS, "Failed to create Vulkan Memory Allocator");

	}
}

#endif