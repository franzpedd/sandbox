#pragma once

#include "Material.h"
#include "Physics/BoundingBox.h"
#include "Util/Math.h"
#include "Util/Memory.h"
#include <string>

namespace Cosmos
{
	// forward declarations
	class Renderer;

	class Mesh
	{
	public:

		// returns a smart-ptr to a new mesh
		static Shared<Mesh> Create(Shared<Renderer> renderer);

		// destructor
		~Mesh() = default;

		// returns the mesh file name
		virtual std::string GetFilepath() const = 0;

		// loads the model from a filepath
		virtual void LoadFromFile(std::string filepath) = 0;
	};
}