#pragma once

#include "Util/Math.h"
#include "Util/Memory.h"
#include <string>

namespace Cosmos
{
	// forward declarations
	class Renderer;
	class Texture2D;

	class Mesh
	{
	public:

		struct Material
		{
			std::string name = { "Base Material" };	// material's name

			std::string colormapPath;				// color-map path
			Shared<Texture2D> colormapTex;			// color-map texture
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
		virtual void OnUpdate(float timestep) = 0;

		// draws the mesh
		virtual void OnRender(void* commandBuffer, glm::mat4& transform, uint64_t id) = 0;

		// loads the model from a filepath
		virtual void LoadFromFile(std::string filepath, float scale = 1.0f) = 0;

	public:

		// sets the mesh as mouse-picked or not
		virtual void SetPicked(bool value) = 0;

	public: // materials

		// modifies the mesh material's colormap
		virtual void SetColormapTexture(std::string filepath) = 0;

		// returns the colormap texture used by the material's mesh
		virtual Shared<Texture2D> GetColormapTexture() = 0;
	};
}