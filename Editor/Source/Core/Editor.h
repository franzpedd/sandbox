#pragma once

#include <Engine.h>

namespace Cosmos
{
	// forward declarations
	class Console;
	class Dockspace;
	class Grid;
	class ImDemo;
	class Menubar;
	class Viewport;

	class Editor : public Application
	{
	public:

		// constructor
		Editor();

		// destructor
		virtual ~Editor();

	private:

		Console* mConsole;
		Dockspace* mDockspace;
		Grid* mGrid;
		ImDemo* mImDemo;
		Menubar* mMenubar;
		Viewport* mViewport;
	};
}