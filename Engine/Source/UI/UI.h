#pragma once

#include "Util/Math.h"
#include "Util/Memory.h"
#include "Util/Stack.h"
#include "Wrapper/imgui.h"

// forward declaration
struct SDL_Window;
union SDL_Event;

namespace Cosmos
{
	// forward declaration
	class Application;
	class Texture2D;
	class Event;
	class Widget;

	class UI
	{
	public:

		// returns a smart-ptr to a new UI
		static Shared<UI> Create(Application* application);

		// constructor
		UI() = default;

		// destructor
		virtual ~UI() = default;

		// returns a reference to the stack of widgets
		inline Stack<Widget*>& GetWidgetStackRef() { return mWidgets; }

	public:

		// sends sdl events to user interface
		static void HandleWindowInputEvent(SDL_Event* e);

		// adds a texture to the ui
		static void* AddTexture(Shared<Texture2D> texture);

	public:

		// adds a new widget to the ui
		void AddWidget(Widget* widget);

		// toggles the cursor on and off
		void ToggleCursor(bool hide);

	public:

		// updates the user interface tick
		virtual void OnUpdate() = 0;

		// event handling
		virtual void OnEvent(Shared<Event> event) = 0;

		// renders ui that contains renderer draws
		virtual void OnRender() = 0;

		// setup imgui backend configuration
		virtual void SetupBackend() = 0;

	protected:

		// setup imgui frontend configuration
		void SetupFrontend();

	public: // custom widgets

		// custom checkbox that slides into enabled/disabled
		static bool CheckboxSliderEx(const char* label, bool* v);

	protected:

		Stack<Widget*> mWidgets;
		ImFont* mIconFA;
		ImFont* mIconLC;
	};
}