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

	private:		

		Shared<Instance> mInstance;
		Shared<Device> mDevice;
		Shared<RenderpassManager> mRenderpassManager;
		Shared<Swapchain> mSwapchain;
		Shared<PipelineLibrary> mPipelineLibrary;
	};
}

#endif