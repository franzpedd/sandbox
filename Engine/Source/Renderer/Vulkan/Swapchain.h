#pragma once
#if defined COSMOS_RENDERER_VULKAN

#include "Util/Memory.h"

#include <volk.h>
#include "Wrapper/vma.h" // include vma after volk

#include <vector>

// forward declarations
namespace Cosmos
{
	class Window;
}

namespace Cosmos::Vulkan
{
	// forward declarations
	class Device;
	class Renderpass;
	class RenderpassManager;

	class Swapchain
	{
	public:

		struct Details
		{
			VkSurfaceCapabilitiesKHR capabilities = {};
			std::vector<VkSurfaceFormatKHR> formats = {};
			std::vector<VkPresentModeKHR> presentModes = {};
		};

	public:

		// constructor
		Swapchain(Shared<Window> window, Shared<Device> device, Shared<RenderpassManager> renderpassManager, uint32_t maxFrames);

		// destructor
		~Swapchain();

	public:

		// returns the vulkan swapchain
		inline VkSwapchainKHR GetSwapchain() const { return mSwapchain; }

		// returns the quantity of swapchain images exists (dual-buffer, tripple-buffer, etc)
		inline uint32_t GetImageCount() const { return mImageCount; }

		// returns a reference to the swapchain images
		inline std::vector<VkImage>& GetImagesRef() { return mImages; }

		// returns a reference to the swapchain image views
		inline std::vector<VkImageView>& GetImageViews() { return mImageViews; }

		// returns a reference to the swapchain surface format
		inline VkSurfaceFormatKHR& GetSurfaceFormat() { return mSurfaceFormat; }

		// returns a reference to the swapchain extent
		inline VkExtent2D& GetExtent() { return mExtent; }

		// returns swapchain's presentation mode
		inline VkPresentModeKHR GetPresentMode() const { return mPresentMode; }

		// returns the swapchain's color view
		inline VkImageView GetColorView() const { return mColorView; }

		// returns the swapchain's depth view
		inline VkImageView GetDepthView() const { return mDepthView; }

		// returns a reference to the available image semaphores
		inline std::vector<VkSemaphore>& GetAvailableSemaphoresRef() { return mImageAvailableSemaphores; }

		// returns a reference to the finished render semaphores
		inline std::vector<VkSemaphore>& GetFinishedSempahoresRef() { return mRenderFinishedSemaphores; }

		// returns a reference to the in flight fences
		inline std::vector<VkFence>& GetInFlightFencesRef() { return mInFlightFences; }

	public:

		// creates the swapchain
		void CreateSwapchain();

		// creates the swapchain image views
		void CreateImageViews();

		// creates the swapchain render pass, a render pass containing the backbuffer
		void CreateRenderPass();

		// creates the swapchain framebuffers
		void CreateFramebuffers();

		// cleans the current swapchain
		void Cleanup();

		// recreates the swapchain
		void Recreate();

		// fills all information about the swapchain details
		Details QueryDetails();

		// creates the syncronization system between gpu-cpu image-rendering
		void CreateSyncSystem();

	public:

		// creates the swapchain command pool
		void CreateCommandPool();

		// creates teh swapchain command buffers
		void CreateCommandBuffers();

	public:

		// returns the optimal surface format
		VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

		// returns the optimal presentation mode
		VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vsync = false);

		// returns the optimal swapchain extent
		VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	private:

		Shared<Window> mWindow;
		Shared<Device> mDevice;
		Shared<RenderpassManager> mRenderpassManager;
		uint32_t mMaxFrames;

		VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
		std::vector<VkImage> mImages = {};
		std::vector<VkImageView> mImageViews = {};
		uint32_t mImageCount = 0;
		VkSurfaceFormatKHR mSurfaceFormat = {};
		VkPresentModeKHR mPresentMode = {};
		VkExtent2D mExtent = {};

		VkImage mColorImage = VK_NULL_HANDLE;
		VmaAllocation mColorMemory = VK_NULL_HANDLE;
		VkImageView mColorView = VK_NULL_HANDLE;
		VkImage mDepthImage = VK_NULL_HANDLE;
		VmaAllocation mDepthMemory = VK_NULL_HANDLE;
		VkImageView mDepthView = VK_NULL_HANDLE;

		// sync system
		std::vector<VkSemaphore> mImageAvailableSemaphores;
		std::vector<VkSemaphore> mRenderFinishedSemaphores;
		std::vector<VkFence> mInFlightFences;
	};
}

#endif