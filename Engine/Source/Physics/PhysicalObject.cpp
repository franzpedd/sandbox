#include "epch.h"
#include "PhysicalObject.h"

#include "PhysicsWorld.h"
#include "Util/Logger.h"

namespace Cosmos::Physics
{
	PhysicalObject::PhysicalObject(Shared<Physics::PhysicsWorld> physicsWorld)
		: mPhysicsWorld(physicsWorld)
	{

	}

	PhysicalObject::~PhysicalObject()
	{
		JPH::BodyInterface& bodyInterface = mPhysicsWorld->GetPhysicsSystemRef().GetBodyInterface();

		bodyInterface.RemoveBody(mBody->GetID());
		bodyInterface.DestroyBody(mBody->GetID());
	}

	void PhysicalObject::LoadSettings(JPH::ShapeSettings& shapeSettings, JPH::Vec3 inPosition, JPH::EMotionType mode, JPH::ObjectLayer layer, JPH::Quat rotation)
	{
		// setup shape configuration
		JPH::BodyInterface& bodyInterface = mPhysicsWorld->GetPhysicsSystemRef().GetBodyInterface();
		JPH::ShapeSettings::ShapeResult result = shapeSettings.Create();
		JPH::ShapeRefC shape = result.Get();

		if (!result.HasError())
		{
			COSMOS_LOG(Logger::Error, "Shape is a nullptr. Error: %s", result.GetError());
			return;
		}

		JPH::BodyCreationSettings bodySettings(shape, inPosition, rotation, mode, layer);

		// setup rigid body
		mBody = bodyInterface.CreateBody(bodySettings);
		bodyInterface.AddBody(mBody->GetID(), JPH::EActivation::DontActivate); // initially objects are not activated

		// refresh broad phase
		mPhysicsWorld->GetPhysicsSystemRef().OptimizeBroadPhase();
	}

	void PhysicalObject::SetVelocity(JPH::Vec3 velocity)
	{
		mPhysicsWorld->GetPhysicsSystemRef().GetBodyInterface().SetLinearVelocity(mBody->GetID(), velocity);
	}

	void PhysicalObject::SetPosition(JPH::Vec3 position, JPH::EActivation activationMode)
	{
		mPhysicsWorld->GetPhysicsSystemRef().GetBodyInterface().SetPosition(mBody->GetID(), position, activationMode);
	}
}