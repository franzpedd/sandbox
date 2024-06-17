#include "Editor.h"

#include "Renderer/Grid.h"

#include "UI/Console.h"
#include "UI/Dockspace.h"
#include "UI/Explorer.h"
#include "UI/ImDemo.h"
#include "UI/Menubar.h"
#include "UI/SceneHierarchy.h"
#include "UI/Viewport.h"

namespace Cosmos
{
	Editor::Editor()
	{
		mDockspace = new Dockspace();
		mImDemo = new ImDemo();
		mConsole = new Console();
		mSceneHierarchy = new SceneHierarchy(mRenderer, mScene);
		mViewport = new Viewport(mWindow, mRenderer, mUI);

		// grid uses viewport's renderpass, must be created after it
		mGrid = new Grid(mRenderer);

		// explorer uses viewport's msaa for rendering images (it's count is 1 for better performance while on editor)
		mExplorer = new Explorer(mRenderer);

		mMenubar = new Menubar(mWindow, mRenderer, mGrid);

		// dockspace must be the first, since we're going to render the scene into the viewport
		mUI->AddWidget(mDockspace); 
		mUI->AddWidget(mMenubar);
		mUI->AddWidget(mImDemo);
		mUI->AddWidget(mViewport);
		mUI->AddWidget(mExplorer);
		mUI->AddWidget(mGrid);
		mUI->AddWidget(mConsole);
		mUI->AddWidget(mSceneHierarchy);
	}

	Editor::~Editor()
	{
		delete mMenubar;
		delete mExplorer;
		delete mGrid;
		delete mViewport;
		delete mSceneHierarchy;
		delete mConsole;
		delete mImDemo;
		delete mDockspace;
	}
}