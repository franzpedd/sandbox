#pragma once

#include "Platform/Input.h"

namespace Cosmos
{
    class Event
    {
    public:

        enum Type
        {
            Undefined = -1,

            // input events
            KeyboardPress,
            KeyboardRelease,
            MousePress,
            MouseRelease,
            MouseWheel,
            MouseMove,

            // window events
            WindowClose,
            WindowResize,

            EventMax
        };

    public:

        // constructor
        Event(const char* name = "Event", Type type = Type::Undefined);

        // destructor
        virtual ~Event() = default;

        // returns the name
        inline const char* GetName() { return mName; }

        // returns the event type
        inline Type GetType() const { return mType; }

    private:

        const char* mName;
        Type mType;
    };

    class KeyboardPressEvent : public Event
    {
    public:

        // constructor
        KeyboardPressEvent(Keycode key, Keymod mod = KEYMOD_NONE);

        // destructor
        ~KeyboardPressEvent() = default;

        // returns the key code 
        inline Keycode GetKeycode() const { return mKeycode; }

        // returns the key modifier
        inline Keymod GetKeymod() const { return mKeymod; }

    private:

        Keycode mKeycode;
        Keymod mKeymod;
    };

    class KeyboardReleaseEvent : public Event
    {
    public:

        // constructor
        KeyboardReleaseEvent(Keycode key, Keymod mod = KEYMOD_NONE);

        // destructor
        ~KeyboardReleaseEvent() = default;

        // returns the key code 
        inline Keycode GetKeycode() const { return mKeycode; }

        // returns the key modifier
        inline Keymod GetKeymod() const { return mKeymod; }

    private:

        Keycode mKeycode;
        Keymod mKeymod;
    };

    class MousePressEvent : public Event
    {
    public:

        // constructor
        MousePressEvent(Buttoncode button);

        // destructor
        ~MousePressEvent() = default;

        // returns the button code 
        inline Buttoncode GetButtoncode() const { return mButtonCode; }

    private:

        Buttoncode mButtonCode;
    };

    class MouseReleaseEvent : public Event
    {
    public:

        // constructor
        MouseReleaseEvent(Buttoncode button);

        // destructor
        ~MouseReleaseEvent() = default;

        // returns the button code 
        inline Buttoncode GetButtoncode() const { return mButtonCode; }

    private:

        Buttoncode mButtonCode;
    };

    class MouseWheelEvent : public Event
    {
    public:

        // constructor
        MouseWheelEvent(int delta);

        // destructor
        ~MouseWheelEvent() = default;

        // returns the delta move
        inline int GetDelta() const { return mDelta; }

    private:

        int mDelta;
    };

    class MouseMoveEvent : public Event
    {
    public:

        // constructor
        MouseMoveEvent(int xOffset, int yOffset);

        // destructor
        ~MouseMoveEvent() = default;

        // returns the new x coodirnate
        inline int GetXOffset() const { return mXOffset; }

        // returns the new y coordinates
        inline int GetYOffset() const { return mYOffset; }

    private:

        int mXOffset;
        int mYOffset;
    };

    class WindowCloseEvent : public Event
    {
    public:

        // constructor
        WindowCloseEvent();

        // destructor
        ~WindowCloseEvent() = default;

    private:

    };

    class WindowResizeEvent : public Event
    {
    public:

        // constructor
        WindowResizeEvent(int width, int height);

        // destructor
        ~WindowResizeEvent() = default;

        // returns the new width 
        inline int GetWidth() const { return mWidth; }

        // returns the new height
        inline int GetHeight() const { return mHeight; }

    private:

        int mWidth;
        int mHeight;
    };
}