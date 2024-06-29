#pragma once
#if defined COSMOS_RENDERER_VULKAN

// changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

#include "Physics/BoundingBox.h"
#include "Renderer/Mesh.h"
#include "Renderer/Vertex.h"
#include "Renderer/Texture.h"
#include "Device.h"

#include "Wrapper/tinygltf.h"
#include <volk.h>

namespace Cosmos::Vulkan
{
	// forward declarations
	class VKRenderer;

	class VKMesh : public Mesh
	{
	public:

		// contains the data for a single draw call
		struct Primitive
		{
			uint32_t firstIndex;
			uint32_t indexCount;
		};

		// contains the node's (optional) geometry and can be made up of an arbitrary number of primitives
		struct Mesh
		{
			std::vector<Primitive> primitives;
		};

		// represents an object in the GLTF scene graph
		struct Node
		{
			Node* parent = nullptr;
			std::vector<Node*> children;
			Mesh mesh;
			glm::mat4 matrix;

			// destructor
			~Node();
		};

	public:

		// constructor
		VKMesh(Shared<VKRenderer> renderer);

		// destructor
		virtual ~VKMesh();

		// returns the mesh file name
		virtual inline std::string GetFilepath() const override { return mFilepath; }

		// returns if the mesh is fully loaded
		virtual inline bool IsLoaded() const override { return mLoaded; }

		// sets the render mode to wiredframe/fill
		virtual bool* GetWiredframe() override { return &mWiredframe; }

	public:

		// updates the mesh logic
		virtual void OnUpdate(float timestep) override;

		// draws the mesh
		virtual void OnRender(void* commandBuffer, glm::mat4& transform, uint32_t id) override;

		// loads the model from a filepath
		virtual void LoadFromFile(std::string filepath, float scale = 1.0f) override;

	public:

		// modifies the mesh material's colormap
		virtual void SetColormapTexture(std::string filepath) override;

		// returns the colormap texture used by the material's mesh
		virtual Shared<Texture2D> GetColormapTexture() override;

	private:

		// loads a node graph from the gltf model
		void LoadGLTFNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);

		// draws a node, including it's children if any
		void DrawNode(Node* node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

		// creates buffers used by the mesh
		void CreateRendererResources(size_t vertexBufferSize, size_t indexBufferSize);

		// creates the uniform buffers and setup descriptors
		void SetupDescriptors();

		// updates the descriptor sets
		void UpdateDescriptors();

	private:

		Shared<VKRenderer> mRenderer;
		std::string mFilepath = {};
		bool mPicked = false;
		bool mLoaded = false;
		bool mWiredframe = false;

		std::vector<Node*> mNodes = {};
		Material mMaterial = {};

		// raw buffers
		std::vector<uint32_t> mRawIndexBuffer = {};
		std::vector<Vertex> mRawVertexBuffer = {};
		
		// gpu buffer
		VkBuffer mVertexBuffer = VK_NULL_HANDLE;
		VmaAllocation mVertexMemory = VK_NULL_HANDLE;
		int mIndicesCount = 0;
		VkBuffer mIndexBuffer = VK_NULL_HANDLE;
		VmaAllocation mIndexMemory = VK_NULL_HANDLE;

		// shader descriptors
		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> mDescriptorSets = {};
	};
}

#endif