#include "Editor.h"

#include "UI/Dockspace.h"
#include "UI/ImDemo.h"
#include "UI/Viewport.h"

namespace Cosmos
{
	Editor::Editor()
	{
		mDockspace = new Dockspace();
		mImDemo = new ImDemo();
		mViewport = new Viewport(mWindow, mRenderer);

		mUI->AddWidget(mDockspace); // dockspace must be the first, since we're going to render the scene into the viewport
		mUI->AddWidget(mImDemo);
		mUI->AddWidget(mViewport);
	}

	Editor::~Editor()
	{
		delete mViewport;
		delete mImDemo;
		delete mDockspace;
	}
}