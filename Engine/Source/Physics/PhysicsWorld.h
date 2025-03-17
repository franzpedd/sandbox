#pragma once

#include "ObjectCollision.h"
#include "Listener.h"
#include "Util/Memory.h"

// forward declarations
namespace Cosmos { class Application; class Event; }

namespace Cosmos::Physics
{
	class PhysicsWorld
	{
	public:

		// constructor
		PhysicsWorld(Application* application);

		// destructor
		~PhysicsWorld();

		// returns a reference to the physics system
		inline JPH::PhysicsSystem& GetPhysicsSystemRef() { return mPhysicsSystem; }

	public:

		// updates the physics world
		void OnUpdate(float timestep);

		// event handling
		void OnEvent(Shared<Event> event);

		// test example
		void RunTest();

	private:

		Application* mApplication;

		JPH::TempAllocatorImpl* mTempAllocator;
		JPH::JobSystemThreadPool* mJobSystem;

		ObjectCollision mObjectCollision;
		BroadPhaseInterface mBPInterface;
		ObjectBroadPhaseCollision mObjectPBCollision;
		JPH::PhysicsSystem mPhysicsSystem;

		OnContactListener mContactListener;
		OnBodyActivationListener mBodyActivationListener;
	};

	// implements a link between cosmos logger and jolt logger
	static void LoggerPhysics(const char* fmt, ...);

	// implements a link between cosmos logger and jolt logger
	static bool  AssertPhysics(const char* expression, const char* message, const char* file, uint32_t line);
}