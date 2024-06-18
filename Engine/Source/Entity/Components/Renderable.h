#pragma once

#include "Renderer/Mesh.h"

namespace Cosmos
{
	struct MeshComponent
	{
		Shared<Mesh> mesh;

		// constructor
		MeshComponent() = default;
	};
}