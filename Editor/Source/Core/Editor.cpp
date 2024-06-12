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

		mUI->GetWidgetStackRef().Push(mDockspace);
		mUI->GetWidgetStackRef().Push(mImDemo);
		mUI->GetWidgetStackRef().Push(mViewport);
	}

	Editor::~Editor()
	{
		delete mViewport;
		delete mImDemo;
		delete mDockspace;
	}
}