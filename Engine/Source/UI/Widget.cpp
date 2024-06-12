#include "epch.h"
#include "Widget.h"

#include "Core/Event.h"

namespace Cosmos
{
	Widget::Widget(const char* name) 
		: mName(name) 
	{
	}

	void Widget::OnUpdate()
	{
	}

	void Widget::OnRender()
	{
	}

	void Widget::OnEvent(Shared<Event> event)
	{
	}
}