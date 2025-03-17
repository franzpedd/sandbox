#include "epch.h"
#include "PhysicsWorld.h"

#include "Core/Application.h"
#include "Core/Event.h"
#include "Util/Logger.h"

#include <cstdarg>
#include <iostream>

namespace Cosmos::Physics
{
	PhysicsWorld::PhysicsWorld(Application* application)
		: mApplication(application)
	{
		// registering default jolt allocator, witch uses malloc and free
		// this must be done before any other jolt function
		JPH::RegisterDefaultAllocator();

		// installing trace and assertions
		JPH::Trace = LoggerPhysics;
		JPH::AssertFailed = AssertPhysics;

		// create a factory, this class is responsable for deserialization of saved data
		JPH::Factory::sInstance = new JPH::Factory();

		// register all physics types with the factory and install their collision handlers
		JPH::RegisterTypes();

		// temp allocator used when updating physics. Allocating 10MB witch is a typical value.
		// it is possible to avoid using this and use TempAllocatorMalloc to fall back to malloc/free
		mTempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

		// temporary job system while I don't have one
		mJobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

		// how many rigid bodies that can be added to the physics system
		constexpr uint32_t maxBodies = 32768;

		// how many mutexes to allocate to protect rigid bodies from concurrent access, 0 for the default settings.
		constexpr uint32_t numBodyMutexes = 0;

		// max amount of body pairs that can be queued at any time (the broad phase will detect overlapping body pairs based on their bounding boxes and will insert them into a queue for the narrowphase)
		constexpr uint32_t maxBodyPairs = 65536;

		// maximum size of the contact constraint buffer.  If more contacts (collisions between bodies) are detected than this number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
		constexpr uint32_t maxContactConstraints = 1024;
		
		// initializes the physics system
		mPhysicsSystem.Init
		(
			maxBodies,
			numBodyMutexes,
			maxBodyPairs,
			maxContactConstraints,
			mBPInterface,
			mObjectPBCollision,
			mObjectCollision
		);

		// a contact listener gets notified when bodies (are about to) collide, and when they separate again
		// note that this is called from a job so whatever you do here needs to be thread safe
		// registering one is entirely optional
		mPhysicsSystem.SetContactListener(&mContactListener);

		// the main way to interact with the bodies in the physics system is through the body interface. 
		// there is a locking and a non-locking variant of thi, we're going to use the locking version
		mPhysicsSystem.SetBodyActivationListener(&mBodyActivationListener);
	}

	PhysicsWorld::~PhysicsWorld()
	{
		// unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		delete JPH::Factory::sInstance;
		delete mJobSystem;
		delete mTempAllocator;
	}

	void PhysicsWorld::OnUpdate(float timestep)
	{
		if (mApplication->GetStatus() == Application::Status::Paused)
			return;

		mPhysicsSystem.Update(timestep, 1, mTempAllocator, mJobSystem);
	}

	void PhysicsWorld::OnEvent(Shared<Event> event)
	{
	}

	void PhysicsWorld::RunTest()
	{
		// gets a reference to the body interface
		JPH::BodyInterface& body_interface = mPhysicsSystem.GetBodyInterface();

		// create a rigid body to serve as the floor, we make a large box
		// create the settings for the collision volume (the shape). Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
		JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 1.0f, 100.0f));
		// a ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.
		floor_shape_settings.SetEmbedded();

		// create the shape
		JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
		// we don't expect an error here, but you can check floor_shape_result for HasError() / GetError()
		JPH::ShapeRefC floor_shape = floor_shape_result.Get();

		// create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
		JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0_r, -1.0_r, 0.0_r), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Static_Layer);

		// create the actual rigid body
		JPH::Body* floor = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

		// add it to the world
		body_interface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);

		// now create a dynamic body to bounce on the floor
		// note that this uses the shorthand version of creating and adding a body to the world
		JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::RVec3(0.0_r, 2.0_r, 0.0_r), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Dynamic_Layer);
		JPH::BodyID sphere_id = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

		// now you can interact with the dynamic body, in this case we're going to give it a velocity.
		// note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system
		body_interface.SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, -5.0f, 0.0f));

		// optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
		// you should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
		// instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
		mPhysicsSystem.OptimizeBroadPhase();

		// we simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system. (Can we?, and is it good?)
		const float cDeltaTime = 1.0f / 60.0f;

		// now we're ready to simulate the body, keep simulating until it goes to sleep
		uint32_t step = 0;
		while (body_interface.IsActive(sphere_id))
		{
			// next step
			++step;

			// output current position and velocity of the sphere
			JPH::RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
			JPH::Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);
			std::cout << "Step " << step << ": Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << std::endl;

			// if you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
			const int cCollisionSteps = 1;

			// step the world
			mPhysicsSystem.Update(cDeltaTime, cCollisionSteps, mTempAllocator, mJobSystem);
		}

		// Remove the sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
		body_interface.RemoveBody(sphere_id);

		// Destroy the sphere. After this the sphere ID is no longer valid.
		body_interface.DestroyBody(sphere_id);

		// Remove and destroy the floor
		body_interface.RemoveBody(floor->GetID());
		body_interface.DestroyBody(floor->GetID());
	}

	void LoggerPhysics(const char* fmt, ...)
	{
		va_list list;
		va_start(list, fmt);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), fmt, list);
		va_end(list);

		COSMOS_LOG(Logger::Trace, buffer);
	}

	bool AssertPhysics(const char* expression, const char* message, const char* file, uint32_t line)
	{
		COSMOS_LOG(Logger::Assert, "Phyiscs Assertion: Expression: %s. Message: %s", expression, message);
	}
}