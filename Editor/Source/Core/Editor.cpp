#include "Editor.h"

#include "UI/ImDemo.h"

namespace Cosmos
{
	Editor::Editor()
	{
		mImDemo = new ImDemo();

		mUI->GetWidgetStackRef().Push(mImDemo);
	}

	Editor::~Editor()
	{
		delete mImDemo;
	}
}