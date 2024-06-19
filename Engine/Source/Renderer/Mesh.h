#pragma once

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

	public:

		// updates the mesh logic
		virtual void OnUpdate(float timestep) = 0;

		// draws the mesh
		virtual void OnRender() = 0;

		// loads the model from a filepath
		virtual void LoadFromFile(std::string filepath, float scale = 1.0f) = 0;
	};
}