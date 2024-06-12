#pragma once

#include "Util/Memory.h"

namespace Cosmos
{
	// forward declaration
	class Event;

	class Widget
	{
	public:

		// constructor
		Widget(const char* name = "Widget");

		// destructor
		virtual ~Widget() = default;

		// returns it's name
		inline const char* Name() { return mName; }

	public:

		// for user interface drawing
		virtual void OnUpdate();

		// for renderer drawing
		virtual void OnRender();

		// // called when the window is resized
		virtual void OnEvent(Shared<Event> event);

	private:

		const char* mName;
	};
}