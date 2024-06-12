#pragma once

#include "Util/Memory.h"

namespace Cosmos
{
	// forward declarations
	class Application;
	class Camera;
	class Event;
	class Window;

	class Renderer
	{
	public:

		// returns a smart-ptr to a new backend renderer
		static Shared<Renderer> Create(Application* application, Shared<Window> window);

		// constructor
		Renderer(Application* application, Shared<Window> window);

		// destructor
		virtual ~Renderer() = default;

	public:

		// returns a smart-ptr to the camera
		inline Shared<Camera> GetCamera() { return mCamera; }

		// returns the current frame 
		inline uint32_t GetCurrentFrame() const { return mCurrentFrame; }

		// returns the current image on the swapchain list of images
		inline uint32_t GetImageIndex() const { return mImageIndex; }

		// returns how many frames are simultaneously rendered
		inline const uint32_t GetConcurrentlyRenderedFramesCount() const { return mConcurrentlyRenderedFrames; }

	public:

		// updates the renderer
		virtual void OnUpdate();

		// event handling
		virtual void OnEvent(Shared<Event> event);
		
	protected:

		Application* mApplication;
		Shared<Window> mWindow;

		Shared<Camera> mCamera;
		uint32_t mCurrentFrame = 0;
		uint32_t mImageIndex = 0;
		const uint32_t mConcurrentlyRenderedFrames = 2;
	};
}