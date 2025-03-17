#pragma once

#include <memory>

#if defined (_WIN32)
#include <Windows.h>
#endif

namespace Cosmos
{
	class IRenderer
	{
	public:

		// returns a smart-ptr to a new backend renderer
		static std::shared_ptr<IRenderer> Create();

		// constructor
		IRenderer() = default;

		// destructor
		virtual ~IRenderer() = default;

	public:

		// initializes the renderer
		virtual void Initialize() = 0;
	
	public:
	
	//	// returns a smart-ptr to the camera
	//	inline Shared<Camera> GetCamera() { return mCamera; }
	//
	//	// returns the current frame 
	//	inline uint32_t GetCurrentFrame() const { return mCurrentFrame; }
	//
	//	// returns the current image on the swapchain list of images
	//	inline uint32_t GetImageIndex() const { return mImageIndex; }
	//
	//	// returns how many frames are simultaneously rendered
	//	inline const uint32_t GetConcurrentlyRenderedFramesCount() const { return mConcurrentlyRenderedFrames; }
	//
	//public:
	//
	//	// updates the renderer
	//	virtual void OnUpdate();
	//
	//	// event handling
	//	virtual void OnEvent(Shared<Event> event);
	//
	//public:
	//
	//	// if using a custom viewport, hint it's size into the renderer
	//	void HintViewportSize(float x, float y) { mViewportSizeX = x; mViewportSizeY = y; }
	//
	//protected:
	//
	//	Application* mApplication;
	//	Shared<Window> mWindow;
	//
	//	Shared<Camera> mCamera;
	//	uint32_t mCurrentFrame = 0;
	//	uint32_t mImageIndex = 0;
	//	const uint32_t mConcurrentlyRenderedFrames = 2;
	//	float mViewportSizeX, mViewportSizeY = 0.0f;

#if defined (_WIN32)

	public:

		// links the renderer with a windows window
		static inline void ConnectWindow(HINSTANCE instance, HWND window);
		
	private:

		static HINSTANCE mInstance;
		static HWND mWindow;
#endif
	};
}