#include "Editor.h"

#include "Renderer/Grid.h"

#include "UI/Console.h"
#include "UI/Dockspace.h"
#include "UI/ImDemo.h"
#include "UI/Menubar.h"
#include "UI/Viewport.h"

namespace Cosmos
{
	Editor::Editor()
	{
		mDockspace = new Dockspace();
		mImDemo = new ImDemo();
		mConsole = new Console();
		mViewport = new Viewport(mWindow, mRenderer);
		// grid uses viewport's renderpass, must be created after it
		mGrid = new Grid(mRenderer); 
		mMenubar = new Menubar(mWindow, mRenderer, mGrid);

		// dockspace must be the first, since we're going to render the scene into the viewport
		mUI->AddWidget(mDockspace); 
		mUI->AddWidget(mMenubar);
		mUI->AddWidget(mImDemo);
		mUI->AddWidget(mViewport);
		mUI->AddWidget(mGrid);
		mUI->AddWidget(mConsole);
	}

	Editor::~Editor()
	{
		delete mGrid;
		delete mViewport;
		delete mMenubar;
		delete mConsole;
		delete mImDemo;
		delete mDockspace;
	}
}