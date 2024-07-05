#include "epch.h"
#include "Listener.h"

#include "Util/Logger.h"

namespace Cosmos::Physics
{
	JPH::ValidateResult OnContactListener::OnContactValidate(const JPH::Body& body0, const JPH::Body& body1, JPH::RVec3Arg baseOffset, const JPH::CollideShapeResult& result)
	{
		COSMOS_LOG(Logger::Trace, "Contact validate callback");

		// allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void OnContactListener::OnContactAdded(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings)
	{
		COSMOS_LOG(Logger::Trace, "A contact was added");
	}

	void OnContactListener::OnContactPersisted(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings)
	{
		COSMOS_LOG(Logger::Trace, "A contact was persisted");
	}

	void OnContactListener::OnContactRemoved(const JPH::SubShapeIDPair& subshapePair)
	{
		COSMOS_LOG(Logger::Trace, "A contact was removed");
	}

	void OnBodyActivationListener::OnBodyActivated(const JPH::BodyID& body, uint64_t data)
	{
		COSMOS_LOG(Logger::Trace, "A body got activated");
	}

	void OnBodyActivationListener::OnBodyDeactivated(const JPH::BodyID& inBodyID, uint64_t inBodyUserData)
	{
		COSMOS_LOG(Logger::Trace, "A body got deactivated");
	}
}