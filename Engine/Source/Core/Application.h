#pragma once

#include "Util/Memory.h"

namespace Cosmos
{
	// forward declarations
	class Event;
	class PhysicsWorld;
	class Renderer;
	class Scene;
	class UI;
	class Window;

	namespace Physics { class PhysicsWorld; }

	class Application
	{
	public:

		enum Status
		{
			Paused,
			Playing
		};

	public:

		// constructor
		Application();

		// destructor
		~Application();

		// returns the current status of the physics world
		inline Status GetStatus() const { return mStatus; }

		// sets a new status for the physics world
		inline void SetStatus(Status status) { mStatus = status; }

		// returns a smart-ptr to the window
		inline Shared<Window> GetWindow() { return mWindow; }

		// returns a smart-ptr to the renderer
		inline Shared<Renderer> GetRenderer() { return mRenderer; }

		// returns a smart-ptr to the user interface
		inline Shared<UI> GetUI() { return mUI; }

		// returns a smart-ptr to the scene
		inline Shared<Scene> GetScene() { return mScene; }

		// returns a smart-ptr to the physics world
		inline Shared<Physics::PhysicsWorld> GetPhysicsWorld() { return mPhysicsWorld; }



	public:

		// main loop
		void Run();

		// event handling
		void OnEvent(Shared<Event> event);

	protected:

		Status mStatus = Status::Paused;
		Shared<Window> mWindow;
		Shared<Renderer> mRenderer;
		Shared<UI> mUI;
		Shared<Scene> mScene;
		Shared<Physics::PhysicsWorld> mPhysicsWorld;
	};
}