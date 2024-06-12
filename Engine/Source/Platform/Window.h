#pragma once

#include "Core/Event.h"
#include "Util/Memory.h"
#include <chrono>

// forward declaration
struct SDL_Window;

namespace Cosmos
{
	// forward declaration
	class Application;

	class Window
	{
	public:

		// constructor
		Window(Application* application, const char* title = "Cosmos", int32_t width = 1280, int32_t height = 720);

		// destructor
		~Window();

		// returns the native window object
		inline SDL_Window* GetNativeWindow() { return mNativeWindow; }

		// returns the average fps count
		inline uint32_t GetFramesPerSecond() const { return mLastFPS; }

		// returns the timestep
		inline float GetTimestep() const { return mTimestep; }

		// returns the window's title
		inline const char* GetTitle() { return mTitle; }

		// returns the window's width
		inline int32_t GetWidth() const { return mWidth; }

		// returns the window's height
		inline int32_t GetHeight() const { return mHeight; }

	public:

		// sets the should quit variable to a value
		inline void HintQuit(bool value) { mShouldQuit = value; }

		// returns if window close event was called
		inline bool ShouldQuit() { return mShouldQuit; }

		// sets the should resize variable to a value
		inline void HintResize(bool value) { mShouldResizeWindow = value; }

		// returns if window resize event was called
		inline bool ShouldResize() { return mShouldResizeWindow; }

	public: // input

		// updates the window input events
		void OnUpdate();

		// enables or disables the cursor
		void ToggleCursor(bool hide);

		// returns if a keyboard keyis pressed
		bool IsKeyPressed(Keycode key);

		// returns if a mouse button is pressed
		bool IsButtonPressed(Buttoncode button);

	public: // window 

		// returns the framebuffer size
		void GetFrameBufferSize(int32_t* width, int32_t* height);

		// recreates the window
		void ResizeFramebuffer();

		// returns the window's aspect ratio
		float GetAspectRatio();

		// returns the instance extensions used by the window
		void GetInstanceExtensions(uint32_t* count, const char** names);

		// creates a window surface for drawing into it
		void CreateSurface(void* instance, void** surface);

	public: // frames per second

		// starts the frames per second count
		void StartFrame();

		// ends the frames per second count
		void EndFrame();

	private:

		Application* mApplication;

		// window object
		SDL_Window* mNativeWindow = nullptr;
		const char* mTitle;
		int32_t mWidth;
		int32_t mHeight;
		bool mShouldQuit = false;
		bool mShouldResizeWindow = false;

		// frames per second system
		std::chrono::high_resolution_clock::time_point mStart;
		std::chrono::high_resolution_clock::time_point mEnd;
		double mTimeDiff = 0.0f;
		float mFpsTimer = 0.0f;
		uint32_t mFrames = 0; // average fps
		float mTimestep = 1.0f; // timestep/delta time (logic updating)
		uint32_t mLastFPS = 0;
	};
}