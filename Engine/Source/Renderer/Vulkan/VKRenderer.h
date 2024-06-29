#pragma once
#if defined COSMOS_RENDERER_VULKAN

#include "Renderer/Renderer.h"
#include <volk.h>
#include "Wrapper/vma.h" // including vma after volk

#include <vector>

// forward declarations
namespace Cosmos
{
	class Window;
}

namespace Cosmos::Vulkan
{
	// forward declarations
	class Application;
	class Device;
	class Instance;
	class PipelineLibrary;
	class RenderpassManager;
	class Swapchain;

	class VKRenderer : public Renderer
	{
	public:

		// constructor
		VKRenderer(Cosmos::Application* application, Shared<Cosmos::Window> window);

		// destructor
		virtual ~VKRenderer();

		// returns a smart-ptr to the vulkan instance class
		inline Shared<Vulkan::Instance> GetInstance() { return mInstance; }

		// returns a smart-ptr to the vulkan device class
		inline Shared<Vulkan::Device> GetDevice() { return mDevice; }

		// returns a smart-ptr to the vulkan render pass manager
		inline Shared<Vulkan::RenderpassManager> GetRenderpassManager() { return mRenderpassManager; }

		// returns a smart-ptr to the vulkan render pass manager
		inline Shared<Vulkan::Swapchain> GetSwapchain() { return mSwapchain; }

		// returns a smart-ptr to the vulkan pipeline library
		inline Shared<Vulkan::PipelineLibrary> GetPipelineLibrary() { return mPipelineLibrary; }

	public:

		// updates the renderer
		virtual void OnUpdate() override;

		// event handling
		virtual void OnEvent(Shared<Event> event) override;

	private:

		// organize the render passes order into the draw command
		void ManageRenderpasses();

		// sends global resources into the renderer
		void SendGlobalResources();

		// create globally used resources
		void CreateGlobalResoruces();

	private:

		Shared<Instance> mInstance;
		Shared<Device> mDevice;
		Shared<RenderpassManager> mRenderpassManager;
		Shared<Swapchain> mSwapchain;
		Shared<PipelineLibrary> mPipelineLibrary;
		
		struct GPUBufferData
		{
			std::vector<VkBuffer> buffers = {};
			std::vector<VmaAllocation> memories = {};
			std::vector<void*> mapped = {};
		};

		GPUBufferData mCameraData;
		GPUBufferData mWindowData;
		GPUBufferData mPickingData;
		uint32_t mPickingID = 0;

	public:

		// returns a reference to the camera global buffer
		inline GPUBufferData& GetCameraDataRef() { return mCameraData; }

		// returns a reference tot he global window buffer
		inline GPUBufferData& GetWindowDataRef() { return mWindowData; }
		
		// returns a reference tot he global picking buffer
		inline GPUBufferData& GetPickingDataRef() { return mPickingData; }

	private:

	};
}

#endif