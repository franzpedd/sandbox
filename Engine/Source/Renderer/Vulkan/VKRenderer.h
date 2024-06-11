#pragma once

#include "Renderer/Renderer.h"
#include "Instance.h"
#include "Device.h"
#include "Renderpass.h"
#include "Swapchain.h"
#include "Pipeline.h"

namespace Cosmos
{
	// forward declarations
	class Application;
	class Window;

	class VKRenderer : public Renderer
	{
	public:

		// constructor
		VKRenderer(Application* application, Shared<Window> window);

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

		// creates the syncronization system used to render ready images
		void CreateSyncSystem();

	private:		

		Shared<Vulkan::Instance> mInstance;
		Shared<Vulkan::Device> mDevice;
		Shared<Vulkan::RenderpassManager> mRenderpassManager;
		Shared<Vulkan::Swapchain> mSwapchain;
		Shared<Vulkan::PipelineLibrary> mPipelineLibrary;

		std::vector<VkSemaphore> mImageAvailableSemaphores;
		std::vector<VkSemaphore> mRenderFinishedSemaphores;
		std::vector<VkFence> mInFlightFences;
	};
}