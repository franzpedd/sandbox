#include "epch.h"
#include "Application.h"

#include "Event.h"
#include "Scene.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include "UI/UI.h"

namespace Cosmos
{
	Application::Application()
	{
		mWindow = CreateShared<Window>(this, "Cosmos", 1280, 720);
		mRenderer = Renderer::Create(this, mWindow);
		mUI = CreateShared<UI>(this);
		mScene = CreateShared<Scene>(mRenderer);
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		while (!mWindow->ShouldQuit())
		{
			mWindow->StartFrame();						// starts the average fps calculation

			// update tick
			mWindow->OnUpdate();						// window input events are processed first
			mUI->OnUpdate();							// renders the user interface second
			mScene->OnUpdate(mWindow->GetTimestep());	// updates the scene logic
			mRenderer->OnUpdate();						// renders current tick

			mWindow->EndFrame();						// finish calculating average fps
		}
	}

	void Application::OnEvent(Shared<Event> event)
	{
		mScene->OnEvent(event);
		mUI->OnEvent(event);
		mRenderer->OnEvent(event);
	}
}