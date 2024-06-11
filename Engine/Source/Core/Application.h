#pragma once

#include "Platform/Window.h"
#include "Renderer/Renderer.h"
#include "UI/UI.h"

namespace Cosmos
{
	class Application
	{
	public:

		// constructor
		Application();

		// destructor
		~Application();

		// returns a smart-ptr to the window
		inline Shared<Window> GetWindow() { return mWindow; }

		// returns a smart-ptr to the renderer
		inline Shared<Renderer> GetRenderer() { return mRenderer; }

		// returns a smart-ptr to the user interface
		inline Shared<UI> GetUI() { return mUI; }

	public:

		// main loop
		void Run();

		// event handling
		void OnEvent(Shared<Event> event);

	private:

		Shared<Window> mWindow;
		Shared<Renderer> mRenderer;
		Shared<UI> mUI;
	};
}