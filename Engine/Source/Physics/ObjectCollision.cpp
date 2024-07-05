#include "epch.h"
#include "ObjectCollision.h"

#include "Util/Logger.h"

namespace Cosmos::Physics
{
	bool ObjectCollision::ShouldCollide(JPH::ObjectLayer obj0, JPH::ObjectLayer obj1) const
	{
		switch (obj0)
		{
			case Static_Layer: // static objects only interacts with dynamic objects
			{
				return obj1 == Dynamic_Layer;
			}

			case Dynamic_Layer: // dynamic objects does interacts with any other objects
			{
				return true;
			}
		}

		COSMOS_LOG(Logger::Error, "Physics: Object is not on the static nor dynamic groups, this is an error.");
		return false;
	}

	BroadPhaseInterface::BroadPhaseInterface()
	{
		// we are mapping the broad phase into 1-1 and this may not be ideal on complex scenarios
		mObjectToBroadPhase[Static_Layer] = Static_BroadPhase;
		mObjectToBroadPhase[Dynamic_Layer] = Dynamic_BroadPhase;
	}

	JPH::BroadPhaseLayer BroadPhaseInterface::GetBroadPhaseLayer(JPH::ObjectLayer layer) const
	{
		COSMOS_ASSERT(layer < BroadPhaseMax, "Outside of broad phase range");
		return mObjectToBroadPhase[layer];
	}
	
	const char* BroadPhaseInterface::GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const
	{
		switch((JPH::BroadPhaseLayer::Type)layer)
		{
			case (JPH::BroadPhaseLayer::Type)Static_BroadPhase: return "Static";
			case (JPH::BroadPhaseLayer::Type)Dynamic_BroadPhase: return "Dynamic";
		}

		return "Invalid";
	}

	bool ObjectBroadPhaseCollision::ShouldCollide(JPH::ObjectLayer object, JPH::BroadPhaseLayer broadphase) const
	{
		switch (object)
		{
			case Static_Layer:
			{
				return broadphase == Dynamic_BroadPhase;
			}
				
			case Dynamic_Layer:
			{
				return true;
			}
		}

		COSMOS_LOG(Logger::Error, "Physics: Object collision with invalid broadphase.");
		return false;
	}
}