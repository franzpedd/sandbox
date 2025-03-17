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

		// returns a ptr that flags the motion type of the object
		inline bool* GetDynamicPtr() { return &mDynamic; }

		// returns the physical object mode
		inline JPH::EMotionType GetMotionType() const { return mMotionType; }

		// sets the motion type of the object
		inline void SetMotionType(JPH::EMotionType type) { mMotionType = type; mDynamic = mMotionType != JPH::EMotionType::Static; }

	public:

		// sets the object phyiscal properties
		void LoadSettings(JPH::ShapeRefC shape, JPH::Vec3 inPosition, JPH::EMotionType mode, JPH::ObjectLayer layer, JPH::Quat rotation);

	public:

		// sets velocity for the physical object
		void SetVelocity(JPH::Vec3 velocity);

		// sets the position of the physical object
		void SetPosition(JPH::Vec3 position, JPH::EActivation activationMode = JPH::EActivation::DontActivate);

	private:

		Shared<Physics::PhysicsWorld> mPhysicsWorld;
		JPH::EMotionType mMotionType = JPH::EMotionType::Static;
		bool mDynamic = false;
		bool mShapeHasBeenSet = false;
		JPH::Body* mBody = nullptr;
		JPH::RVec3 mCurrentPosition = {};
		JPH::Vec3 mCurrentVelocity = {};
	};
}