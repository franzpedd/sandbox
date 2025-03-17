#pragma once

#include "Vertex.h"
#include "Util/Math.h"
#include "Util/Memory.h"
#include <string>
#include <vector>

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
			int32_t index = 0;						// used when a mesh contain more than one material
			std::string name = { "Base Material" };	// material's name
			std::string colormapPath;				// color-map path
			Shared<Texture2D> colormapTex;			// color-map texture
		};

		struct Dimension
		{
			glm::mat4 aabb;
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
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

		// returns the vector of vertices of the mesh
		virtual std::vector<Vertex> GetVertices() const = 0;

	public:

		// updates the mesh logic
		virtual void OnUpdate(float timestep) = 0;

		// draws the mesh
		virtual void OnRender(void* commandBuffer, glm::mat4& transform, uint32_t id) = 0;

		// loads the model from a filepath
		virtual void LoadFromFile(std::string filepath, float scale = 1.0f) = 0;

		// returns the mesh dimension
		virtual Dimension GetDimension() const = 0;

	public: // materials

		// modifies the mesh material's colormap
		virtual void SetColormapTexture(std::string filepath) = 0;
		
		// returns the colormap texture used by the material's mesh
		virtual Shared<Texture2D> GetColormapTexture() = 0;
	};
}