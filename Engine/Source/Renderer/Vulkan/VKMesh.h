#pragma once
#if defined COSMOS_RENDERER_VULKAN

#include "Renderer/Mesh.h"
#include "Renderer/Vertex.h"
#include "Renderer/Texture.h"

#include "Wrapper/tinygltf.h"
#include <volk.h>

namespace Cosmos::Vulkan
{
	// forward declarations
	class VKRenderer;

	class VKMesh : public Mesh
	{
	public:

		// helper, will become members when finished
		struct LoaderInfo
		{
			uint32_t* indexbuffer;
			size_t indexpos = 0;

			Vertex* vertexbuffer;
			size_t vertexpos = 0;
		};

	public:

		// constructor
		VKMesh(Shared<VKRenderer> renderer);

		// destructor
		virtual ~VKMesh();

		// returns the mesh file name
		virtual inline std::string GetFilepath() const override { return mFilepath; }

	public:

		// loads the model from a filepath
		virtual void LoadFromFile(std::string filepath) override;

	private:

		// interprets the mesh node, reading it's vertex and index count
		void GetVertexAndIndicesCount(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount);


	private:

		// testing 
		void LoadSamplers(tinygltf::Model& model);
		void LoadTextures(tinygltf::Model& model);
		void LoadMaterials(tinygltf::Model& model);

	public:

		Shared<VKRenderer> mRenderer;
		std::string mFilepath = {};

		std::vector<TextureSampler> mSamplers;
		std::vector<Shared<Texture2D>> mTextures;
		std::vector<Material> mMaterials;
	};
}

#endif