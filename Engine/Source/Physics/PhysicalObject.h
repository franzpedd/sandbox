#pragma once

#include "Util/Memory.h"
#include "Wrapper/jolt.h"

namespace Cosmos::Physics
{
	// forward declarations
	class PhysicsWorld;

	class PhysicalObject
	{
	public:

		// constructor
		PhysicalObject(Shared<Physics::PhysicsWorld> physicsWorld);

		// destructor
		~PhysicalObject();

		// returns a reference to the physical object mode, allowing to modify it
		inline JPH::EMotionType& GetMotionTypeRef() { return mMotionType; }

	public:

		// sets the object phyiscal properties
		void LoadSettings(JPH::ShapeSettings& shapeSettings, JPH::Vec3 inPosition, JPH::EMotionType mode, JPH::ObjectLayer layer, JPH::Quat rotation = JPH::Quat::sIdentity());

	public:

		// sets velocity for the physical object
		void SetVelocity(JPH::Vec3 velocity);

		// sets the position of the physical object
		void SetPosition(JPH::Vec3 position, JPH::EActivation activationMode = JPH::EActivation::DontActivate);

	private:

		Shared<Physics::PhysicsWorld> mPhysicsWorld;
		JPH::EMotionType mMotionType = JPH::EMotionType::Static;
		bool mShapeHasBeenSet = false;
		JPH::Body* mBody = nullptr;
		JPH::RVec3 mCurrentPosition = {};
		JPH::Vec3 mCurrentVelocity = {};
	};
}