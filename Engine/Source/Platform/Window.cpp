#include "epch.h"
#include "Window.h"

#include "Core/Application.h"
#include "Platform/Detection.h"
#include "UI/UI.h"

#if defined(PLATFORM_WINDOWS)
#pragma warning(push)
#pragma warning(disable : 26819)
#endif

#if defined(PLATFORM_LINUX)
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_scancode.h>
#elif defined(PLATFORM_WINDOWS)
#include <SDL.h>
#include <SDL_vulkan.h>
#include <SDL_scancode.h>
#endif

#if defined(PLATFORM_WINDOWS)
#pragma warning(pop)
#endif

namespace Cosmos
{
	Window::Window(Application* application, const char* title, int32_t width, int32_t height)
		: mApplication(application), mTitle(title), mWidth(width), mHeight(height)
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
		{
			SDL_Log("Could not initialize SDL. Error: %s", SDL_GetError());
			return;
		}

		SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

		mNativeWindow = SDL_CreateWindow
		(
			mTitle,
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			mWidth,
			mHeight,
			SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
		);

		if (!mNativeWindow)
		{
			SDL_Log("Could not create SDL Window. Error: %s", SDL_GetError());
			return;
		}
	}

	Window::~Window()
	{
		SDL_DestroyWindow(mNativeWindow);
		SDL_Quit();
	}
	
	void Window::OnUpdate()
	{
		SDL_Event SDL_E;

		while (SDL_PollEvent(&SDL_E))
		{
            // process event on the ui
            UI::HandleInternalEvent(&SDL_E);

			switch (SDL_E.type)
			{ 
                // input
                case SDL_KEYDOWN:
                {
                    Shared<KeyboardPressEvent> event = CreateShared<KeyboardPressEvent>((Keycode)SDL_E.key.keysym.sym, (Keymod)SDL_E.key.keysym.mod);
                    mApplication->OnEvent(event);

                    break;
                }

                case SDL_KEYUP:
                {
                    Shared<KeyboardReleaseEvent> event = CreateShared<KeyboardReleaseEvent>((Keycode)SDL_E.key.keysym.sym, (Keymod)SDL_E.key.keysym.mod);
                    mApplication->OnEvent(event);

                    break;
                }

                case SDL_MOUSEBUTTONDOWN:
                {
                    Shared<MousePressEvent> event = CreateShared<MousePressEvent>((Buttoncode)SDL_E.button.button);
                    mApplication->OnEvent(event);

                    break;
                }

                case SDL_MOUSEBUTTONUP:
                {
                    Shared<MouseReleaseEvent> event = CreateShared<MouseReleaseEvent>((Buttoncode)SDL_E.button.button);
                    mApplication->OnEvent(event);

                    break;
                }

                case SDL_MOUSEWHEEL:
                {
                    Shared<MouseWheelEvent> event = CreateShared<MouseWheelEvent>(SDL_E.wheel.y);
                    mApplication->OnEvent(event);

                    break;
                }

                case SDL_MOUSEMOTION:
                {
                    Shared<MouseMoveEvent> event = CreateShared<MouseMoveEvent>(SDL_E.motion.xrel, SDL_E.motion.yrel);
                    mApplication->OnEvent(event);

                    break;
                }

                // window
                case SDL_QUIT:
                {
                    mShouldQuit = true;

                    Shared<WindowCloseEvent> event = CreateShared<WindowCloseEvent>();
                    mApplication->OnEvent(event);

                    break;
                }

                case SDL_WINDOWEVENT:
                {
                    if (SDL_E.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || SDL_E.window.event == SDL_WINDOWEVENT_MINIMIZED)
                    {
                        HintResize(true);
                    
                        Shared<WindowResizeEvent> event = CreateShared<WindowResizeEvent>(SDL_E.window.data1, SDL_E.window.data2);
                        mApplication->OnEvent(event);
                    }

                    break;
                }

                // not handling other events
			    default:
                {
                    break;
                }
			}
		}
	}

    void Window::ToggleCursor(bool hide)
    {
        if (hide)
        {
            SDL_ShowCursor(SDL_DISABLE);
            SDL_SetRelativeMouseMode(SDL_TRUE);
        }

        else
        {
            SDL_ShowCursor(SDL_ENABLE);
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
    }

    bool Window::IsKeyPressed(Keycode key)
    {
        const uint8_t* keys = SDL_GetKeyboardState(nullptr);
        return keys[(SDL_Scancode)key];
    }

    bool Window::IsButtonPressed(Buttoncode button)
    {
        int32_t x, y;

        if (SDL_GetMouseState(&x, &y) & SDL_BUTTON((uint32_t)button)) return true;
        return false;
    }

    void Window::GetFrameBufferSize(int32_t* width, int32_t* height)
    {
        SDL_Vulkan_GetDrawableSize(mNativeWindow, width, height);
    }

    void Window::Recreate()
    {
        SDL_Event e;

        int32_t width = 0;
        int32_t height = 0;
        SDL_Vulkan_GetDrawableSize(mNativeWindow, &width, &height);

        // this is wacky but SDL_Vulkan will contain invalid data when the window is minimized, 
        // so we must stale the window somehow, I have choose to wait the application but this will not be viable for the future
        // therefore I must consider creating a swapchain with invalid size?
        while(SDL_GetWindowFlags(mNativeWindow) & SDL_WINDOW_MINIMIZED)
        {
            SDL_WaitEvent(&e);
        }
    }

    float Window::GetAspectRatio()
    {
        int32_t width = 0;
        int32_t height = 0;
        GetFrameBufferSize(&width, &height);

        if (height == 0) // avoid division by 0
        {
            return 1.0f;
        }

        float aspect = ((float)width / (float)height);
        return aspect;
    }

    void Window::GetInstanceExtensions(uint32_t* count, const char** names)
    {
        SDL_Vulkan_GetInstanceExtensions(mNativeWindow, count, names);
    }

    void Window::CreateSurface(void* instance, void** surface)
    {
        if (!SDL_Vulkan_CreateSurface(mNativeWindow, (VkInstance)instance, (VkSurfaceKHR*)(surface)))
        {
            SDL_Log("Error when creating SDL Window Surface for Vulkan. Error: %s", SDL_GetError());
        }
    }

    void Window::StartFrame()
    {
        mStart = std::chrono::high_resolution_clock::now();
    }

    void Window::EndFrame()
    {
        mEnd = std::chrono::high_resolution_clock::now(); // ends timer
        mFrames++; // add frame to the count

        // calculates time taken by the renderer updating
        mTimeDiff = std::chrono::duration<double, std::milli>(mEnd - mStart).count();

        mTimestep = (float)mTimeDiff / 1000.0f; // timestep

        // calculates time taken by last timestamp and renderer finished
        mFpsTimer += (float)mTimeDiff;

        if (mFpsTimer > 1000.0f) // greater than next frame, reset frame counting
        {
            mLastFPS = (uint32_t)((float)mFrames * (1000.0f / mFpsTimer));
            mFrames = 0;
            mFpsTimer = 0.0f;
        }
    }
}