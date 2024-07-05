#pragma once

#include "Physics/PhysicalObject.h"

namespace Cosmos
{
	struct PhysicsComponent
	{
		Shared<Physics::PhysicalObject> object;

		// constructor
		PhysicsComponent() = default;
	};
}