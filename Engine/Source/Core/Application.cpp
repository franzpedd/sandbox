#include "epch.h"
#include "Application.h"

namespace Cosmos
{
	Application::Application()
	{
		mWindow = CreateShared<Window>(this, "Cosmos", 1280, 720);
		mRenderer = Renderer::Create(this, mWindow);
		mUI = CreateShared<UI>(this);
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		while (!mWindow->ShouldQuit())
		{
			mWindow->StartFrame();	// starts the average fps calculation

			// update tick
			mWindow->OnUpdate();	// window input events are processed first

			// update scene, entities, ui, etc...
			mUI->OnUpdate();		// renders the user interface
			mRenderer->OnUpdate();	// renders current tick

			mWindow->EndFrame();	// finish calculating average fps
		}
	}

	void Application::OnEvent(Shared<Event> event)
	{
		mUI->OnEvent(event);
		mRenderer->OnEvent(event);
	}
}