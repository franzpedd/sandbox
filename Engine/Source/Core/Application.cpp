#include "epch.h"
#include "Application.h"

#include "Event.h"
#include "Scene.h"
#include "Physics/PhysicsWorld.h"
#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include "UI/UI.h"

#include "IRenderer.h"

#include "Platform/Detection.h"

#include <SDL_syswm.h>

namespace Cosmos
{
	Application::Application()
	{
		//mPhysicsWorld = CreateShared<Physics::PhysicsWorld>(this);
		mWindow = CreateShared<Window>(this, "Cosmos", 1280, 720);
		//mRenderer = Renderer::Create(this, mWindow);
		//mUI = UI::Create(this);
		//mScene = CreateShared<Scene>(mRenderer);

		SDL_SysWMinfo sys;
		mWindow->GetSystemInformation(&sys);
		IRenderer::ConnectWindow(sys.info.win.hinstance, sys.info.win.window);

		Shared<IRenderer> renderer = IRenderer::Create();
		renderer->Initialize();
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		//while (!mWindow->ShouldQuit())
		//{
		//	mWindow->StartFrame();								// starts the average fps calculation
		//
		//	// update tick
		//	mWindow->OnUpdate();								// window input events are processed first
		//	mUI->OnUpdate();									// renders the user interface second
		//	mScene->OnUpdate(mWindow->GetTimestep());			// updates the scene logic
		//	mPhysicsWorld->OnUpdate(mWindow->GetTimestep());	// updates the physics
		//	mRenderer->OnUpdate();								// renders current tick
		//
		//	mWindow->EndFrame();								// finish calculating average fps
		//}
	}

	void Application::OnEvent(Shared<Event> event)
	{
		//mScene->OnEvent(event);
		//mUI->OnEvent(event);
		//mPhysicsWorld->OnEvent(event);
		//mRenderer->OnEvent(event);
	}
}