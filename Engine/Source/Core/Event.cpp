#include "epch.h"
#include "Event.h"

namespace Cosmos
{
	Event::Event(const char* name, Type type)
		: mName(name), mType(type)
	{
	}

	KeyboardPressEvent::KeyboardPressEvent(Keycode key, Keymod mod)
		: Event("Keyboard Press", Type::KeyboardPress), mKeycode(key), mKeymod(mod)
	{
	}

	KeyboardReleaseEvent::KeyboardReleaseEvent(Keycode key, Keymod mod)
		: Event("Keyboard Release", Type::KeyboardRelease), mKeycode(key), mKeymod(mod)
	{
	}

	MousePressEvent::MousePressEvent(Buttoncode button)
		: Event("Mouse Press", Type::MousePress), mButtonCode(button)
	{
	}

	MouseReleaseEvent::MouseReleaseEvent(Buttoncode button)
		: Event("Mouse Release", Type::MouseRelease), mButtonCode(button)
	{
	}
	
	MouseWheelEvent::MouseWheelEvent(int delta)
		: Event("Mouse Wheel", Type::MouseWheel), mDelta(delta)
	{
	}

	MouseMoveEvent::MouseMoveEvent(int xOffset, int yOffset)
		: Event("Mouse Move", Type::MouseMove), mXOffset(xOffset), mYOffset(yOffset)
	{
	}

	WindowCloseEvent::WindowCloseEvent()
		: Event("Window Close", Type::WindowClose)
	{
	}

	WindowResizeEvent::WindowResizeEvent(int width, int height)
		: Event("Window Resize", Type::WindowResize), mWidth(width), mHeight(height)
	{
	}
}