#pragma once

#include "Wrapper/jolt.h"

namespace Cosmos::Physics
{
	// listener to recieve collision events
	class OnContactListener : public JPH::ContactListener
	{
	public:

		// event-called when contact is validated
		virtual JPH::ValidateResult OnContactValidate(const JPH::Body& body0, const JPH::Body& body1, JPH::RVec3Arg baseOffset, const JPH::CollideShapeResult& result) override;

		// event-called when contact is added
		virtual void OnContactAdded(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings) override;

		// event-called when contact persisted
		virtual void OnContactPersisted(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings) override;

		// event-called when contact is removed
		virtual void OnContactRemoved(const JPH::SubShapeIDPair& subshapePair) override;
	};

	// listener to recieve activation/deativation events
	class OnBodyActivationListener : public JPH::BodyActivationListener
	{
	public:

		// event-called when body is activated
		virtual void OnBodyActivated(const JPH::BodyID& body, uint64_t data) override;

		// event-called when body is deativated
		virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, uint64_t inBodyUserData) override;
	};
}