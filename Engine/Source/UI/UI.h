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
	class Event;
	class Widget;

	class UI
	{
	public:

		// constructor
		UI(Application* application);

		// destructor
		~UI();

		// returns a reference to the stack of widgets
		inline Stack<Widget*>& GetWidgetStackRef() { return mWidgets; }

	public:

		// updates the user interface tick
		void OnUpdate();

		// renders ui that contains renderer draws
		void OnRender();

		// event handling
		void OnEvent(Shared<Event> event);

		// sends the command buffer to be drawn
		void Draw(void* commandBuffer);

	public:

		// adds a new widget into the ui
		void AddWidget(Widget* widget);

		// sets the minimum image count, used whenever the swapchain is resized and image count change
		void SetImageCount(uint32_t count);

	public:

		// enables/disables cursor
		static void ToggleCursor(bool hide);

		// sends sdl events to user interface
		static void HandleInternalEvent(SDL_Event* e);

	public:

		// adds a texture into the ui
		static void* AddTexture(void* sampler, void* view);

		// removes a texture from the ui
		static void RemoveTexture(void* descriptor);

	private:

		// ui configuration
		void SetupConfiguration();

		// create vulkan resources
		void CreateResources();

	public: // custom widgets

		// custom checkbox that slides into enabled/disabled
		static bool CheckboxSliderEx(const char* label, bool* v);

	private:

		Application* mApplication;
		Stack<Widget*> mWidgets;
		ImFont* mIconFA;
		ImFont* mIconLC;
	};
}