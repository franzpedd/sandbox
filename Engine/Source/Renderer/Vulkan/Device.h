#pragma once
#if defined COSMOS_RENDERER_VULKAN

#include "Util/Memory.h"

#include <volk.h>
#include "Wrapper/vma.h" // including vma after volk

#include <optional>
#include <vector>

// forward declarations
namespace Cosmos
{
	class Window;
}

namespace Cosmos::Vulkan
{
	// forward declarations
	class Instance;
	
	class Device
	{
	public:

		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphics;
			std::optional<uint32_t> present;
			std::optional<uint32_t> compute;

			// returns if found all queues
			inline bool IsComplete() const { return graphics.has_value() && present.has_value() && compute.has_value(); }
		};

	public:

		// constructor
		Device(Shared<Window> window, Shared<Instance> instance);

		// destructor
		~Device();

		// returns a reference to the vulkan surface
		inline VkSurfaceKHR GetSurface() const { return mSurface; }

		// returns a reference to the vulkan device
		inline VkDevice GetLogicalDevice() const { return mDevice; }

		// returns a reference to the vulkan physical device
		inline VkPhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }

		// returns the graphics queue
		inline VkQueue GetGraphicsQueue() const { return mGraphicsQueue; }

		// returns the presentation queue
		inline VkQueue GetPresentQueue() const { return mPresentQueue; }

		// returns the compute queue
		inline VkQueue GetComputeQueue() const { return mComputeQueue; }

		// returns the sampling in use
		inline VkSampleCountFlagBits GetMSAA() const { return mMSAACount; }

		// returns the vulkan memory allocator
		inline VmaAllocator GetAllocator() const { return mAllocator; }

		// returns a reference to the vulkan physical device features
		inline VkPhysicalDeviceFeatures& GetFeaturesRef() { return mFeatures; }

		// returns a reference to the vulkan physical device properties
		inline VkPhysicalDeviceProperties& GetPropertiesRef() { return mProperties; }

		// returns a reference to the vulkan physical device memory properties
		inline VkPhysicalDeviceMemoryProperties& GetMemoryPropertiesRef() { return mMemoryProperties; }

	public: // device

		// returns the queue indices for all available queues
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

		// returns the maximum MSAA sample the physical device handles
		VkSampleCountFlagBits GetMaxUsableSamples();

		// returns the index of requested type of memory
		uint32_t GetMemoryType(uint32_t bits, VkMemoryPropertyFlags properties, VkBool32* found = nullptr);

		// creates a memory buffer on gpu based on parameters
		VkResult CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceSize size, VkBuffer* buffer, VmaAllocation* memory, void* data = nullptr);

	public: // command buffer

		// creates a command buffer given a command pool
		VkCommandBuffer CreateCommandBuffer(VkCommandPool cmdPool, VkCommandBufferLevel level, bool begin = false);

		// initializes the command buffer
		void BeginCommandBuffer(VkCommandBuffer cmdBuffer);

		// finishes the recording of a command buffer and send it to the queue
		void EndCommandBuffer(VkCommandPool cmdPool, VkCommandBuffer cmdBuffer, VkQueue queue, bool free = false);

		// starts the recording of a once-used command buffer
		VkCommandBuffer BeginSingleTimeCommand(VkCommandPool commandPool);

		// ends the recording of a once-used command buffer
		void EndSingleTimeCommand(VkCommandPool commandPool, VkCommandBuffer commandBuffer);

	public: // image

		// returns the most suitable format from candidates (given tiling and features)
		VkFormat FindSuitableFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

		// returns a suitable depth format supported by the device
		VkFormat FindSuitableDepthFormat();

		// creates an image based on specification
		void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arrayLayers, VkSampleCountFlagBits samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VmaAllocation& allocation, VkImageCreateFlags flags = 0, VmaAllocationCreateFlags vmaFlags = 0);

		// creates an image view based on specification
		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevel = 1, uint32_t layerCount = 1, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

		// create a sampler based on specification
		VkSampler CreateSampler(VkFilter min, VkFilter mag, VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w, float mipLevels = 1.0f);

		// modifies an image layout to a new one
		void TransitionImageLayout(VkCommandPool& cmdPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1, uint32_t layerCount = 1);

		// creates an image memory barrier
		void InsertImageMemoryBarrier(VkCommandBuffer cmdbuffer, VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subresourceRange);

	private:

		// selects the most suitable physical device available
		void SelectPhysicalDevice();

		// creates a logical device out of the choosen physical device
		void CreateLogicalDevice();

		// creates the vma
		void CreateAllocator();

	private:

		Shared<Window> mWindow;
		Shared<Instance> mInstance;

		VkSurfaceKHR mSurface = VK_NULL_HANDLE;
		VkDevice mDevice = VK_NULL_HANDLE;
		VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties mProperties = {};
		VkPhysicalDeviceFeatures mFeatures = {};
		VkPhysicalDeviceMemoryProperties mMemoryProperties = {};
		VkQueue mGraphicsQueue = VK_NULL_HANDLE;
		VkQueue mPresentQueue = VK_NULL_HANDLE;
		VkQueue mComputeQueue = VK_NULL_HANDLE;
		VkSampleCountFlagBits mMSAACount = VK_SAMPLE_COUNT_1_BIT;
		VmaAllocator mAllocator = VK_NULL_HANDLE;
	};
}

#endif