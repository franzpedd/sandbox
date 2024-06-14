#pragma once
#if defined COSMOS_RENDERER_VULKAN

#include "UI/UI.h"

// forward declaration
namespace Cosmos
{
	class Application;
}

namespace Cosmos::Vulkan
{
	class VKUI : public UI
	{
	public:

		// constructor
		VKUI(Cosmos::Application* application);

		// destructor
		~VKUI();

	public:

		// updates the user interface tick
		virtual void OnUpdate() override;

		// event handling
		virtual void OnEvent(Shared<Event> event) override;

		// renders ui that contains renderer draws
		virtual	void OnRender() override;

	protected:

		// setup imgui backend configuration
		virtual void SetupBackend() override;

	public:

		// draws the ui based on the commandbuffer
		void Draw(void* commandBuffer);

		// sets the minimum image count, used whenever the swapchain is resized and image count change
		void SetImageCount(uint32_t count);

	private:

		// create vulkan resources
		void CreateResources();

	public:

		// sends sdl events to user interface
		static void HandleSDLEvent(SDL_Event* e);

		// adds a texture to the ui
		static void* AddTextureBackend(Shared<Texture2D> texture);

		// adds a texture to the ui, using vulkan objects
		static void* AddTextureBackend(void* sampler, void* view);

		// removes a texture from the ui
		static void RemoveTexture(void* descriptor);

	private:

		Cosmos::Application* mApplication;
	};
}

#endif