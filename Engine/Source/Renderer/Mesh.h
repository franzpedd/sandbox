#pragma once

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

		enum RenderMode
		{
			Fill = 0, Wiredframe
		};

	public:

		// returns a smart-ptr to a new mesh
		static Shared<Mesh> Create(Shared<Renderer> renderer);

		// destructor
		~Mesh() = default;

		// returns the mesh file name
		virtual std::string GetFilepath() const = 0;

		// returns if the mesh is fully loaded
		virtual bool IsLoaded() const = 0;

		// gets the render mode to wiredframe/fill
		virtual bool* GetWiredframe() = 0;

	public:

		// updates the mesh logic
		virtual void OnUpdate(float timestep, glm::mat4& transform) = 0;

		// draws the mesh
		virtual void OnRender() = 0;

		// loads the model from a filepath
		virtual void LoadFromFile(std::string filepath, float scale = 1.0f) = 0;
	};
}