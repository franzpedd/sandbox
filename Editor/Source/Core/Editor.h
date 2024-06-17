#pragma once

#include <Engine.h>

namespace Cosmos
{
	// forward declarations
	class Console;
	class Dockspace;
	class Explorer;
	class Grid;
	class ImDemo;
	class Menubar;
	class SceneHierarchy;
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
		Explorer* mExplorer;
		Grid* mGrid;
		ImDemo* mImDemo;
		Menubar* mMenubar;
		SceneHierarchy* mSceneHierarchy;
		Viewport* mViewport;
	};
}