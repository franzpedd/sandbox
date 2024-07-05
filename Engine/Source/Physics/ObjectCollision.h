#pragma once

#include "Wrapper/jolt.h"

namespace Cosmos::Physics
{
	// layer Groups that an object may belong to
	static constexpr JPH::ObjectLayer Static_Layer = 0;	// static objects cannot interact with any other objects
	static constexpr JPH::ObjectLayer Dynamic_Layer = 1;	// dynamic objects can interact with any other objects
	static constexpr JPH::ObjectLayer Layer_Max = 2;		// max number of layers

	// broad phase group divided into dynamic and static to update only the necessary ones
	static constexpr JPH::BroadPhaseLayer  Static_BroadPhase(0);
	static constexpr JPH::BroadPhaseLayer  Dynamic_BroadPhase(1);
	static constexpr uint32_t  BroadPhaseMax = 2; // max number of layers

	// determines collision between objects
	class ObjectCollision : public JPH::ObjectLayerPairFilter
	{
	public:

		// determines if two objects should collide with each other
		virtual bool ShouldCollide(JPH::ObjectLayer obj0, JPH::ObjectLayer obj1) const override;
	};

	// defines mapping between object and broadphase layers
	class BroadPhaseInterface : public JPH::BroadPhaseLayerInterface
	{
	public:

		// constructor
		BroadPhaseInterface();

		// returns the number of broad phase layers
		virtual inline uint32_t GetNumBroadPhaseLayers() const override { return BroadPhaseMax; }

		// returns a broadphase layer
		virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override;

		// returns the name of the broad phase layer
		const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const;

	private:

		JPH::BroadPhaseLayer mObjectToBroadPhase[BroadPhaseMax];
	};

	// determines collision between objects and broad phase layers
	class ObjectBroadPhaseCollision : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:

		// determines if object should collide with broadphase
		virtual bool ShouldCollide(JPH::ObjectLayer object, JPH::BroadPhaseLayer broadphase) const override;
	};
}