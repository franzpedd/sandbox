#include "epch.h"
#include "Renderer.h"

#include "Entity/Unique/Camera.h"
#include "Vulkan/VKRenderer.h"

namespace Cosmos
{
	Shared<Renderer> Renderer::Create(Application* application, Shared<Window> window)
	{
		return CreateShared<Vulkan::VKRenderer>(application, window);
	}

	Renderer::Renderer(Application* application, Shared<Window> window)
		: mApplication(application), mWindow(window)
	{
		mCamera = CreateShared<Camera>(window);
	}

	void Renderer::OnUpdate()
	{
		// camera may be required to perform certain optimizations, so we first update it's logic for further use
		mCamera->OnUpdate();
	}

	void Renderer::OnEvent(Shared<Event> event)
	{
		mCamera->OnEvent(event);
	}
}