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

// forward declarations
namespace Cosmos::Vulkan { class VKRenderer; }

namespace Cosmos::Vulkan::GLTF
{
	// forward declaration
	struct Node;

	struct Skin
	{
		std::string name;
		Node* skeletonRoot = nullptr;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<Node*> joints;
	};

	struct Primitive
	{
		uint32_t firstIndex;
		uint32_t indexCount;
		uint32_t vertexCount;
		Cosmos::Mesh::Material& material;
		bool hasIndices;
		Physics::BoundingBox bb;

		// constructor
		Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Cosmos::Mesh::Material& material);

		// sets the bounding box of the primitive
		void SetBoundingBox(glm::vec3 min, glm::vec3 max);
	};

	struct Mesh
	{
		struct UniformBuffer
		{
			VkBuffer buffer;
			VmaAllocation memory;
			VkDescriptorBufferInfo descriptor;
			VkDescriptorSet descriptorSet;
			void* mapped;
		};

		struct UniformBlock
		{
			glm::mat4 matrix = glm::mat4(1.0f);
			glm::mat4 jointMatrix[MAX_NUM_JOINTS] = {};
			uint32_t jointcount{ 0 };
		};

		Shared<Cosmos::Vulkan::Device> device;
		std::vector<Primitive*> primitives;
		Physics::BoundingBox bb;
		Physics::BoundingBox aabb;
		UniformBuffer uniformBuffer = {};
		UniformBlock uniformBlock = {};

		// constructor
		Mesh(Shared<Cosmos::Vulkan::Device> device, glm::mat4 matrix);

		// destructor
		~Mesh();

		// sets the mesh bounding box
		void SetBoundingBox(glm::vec3 min, glm::vec3 max);
	};

	struct Node
	{
		Node* parent = nullptr;
		uint32_t index = 0;
		std::vector<Node*> children = {};
		glm::mat4 matrix = glm::mat4(1.0f);
		std::string name = {};
		Mesh* mesh = nullptr;
		Skin* skin = nullptr;
		int32_t skinIndex = -1;
		glm::vec3 translation = glm::vec3(1.0f);
		glm::vec3 scale = glm::vec3(1.0f);
		glm::quat rotation = {};
		Physics::BoundingBox bvh = {};
		Physics::BoundingBox aabb = {};

		// constructor
		Node() = default;

		// destructor
		~Node();

		// calculate the node root matrix
		glm::mat4 GetLocalMatrix();

		// returns the node matrix, accounting it's leafs
		glm::mat4 GetMatrix();

		// updates the node position
		void OnUpdate();
	};

	struct Animation
	{
		struct Channel
		{
			enum PathType { TRANSLATION, ROTATION, SCALE };
			PathType path;
			Node* node;
			uint32_t samplerIndex;
		};

		struct Sampler
		{
			enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
			InterpolationType interpolation;
			std::vector<float> inputs;
			std::vector<glm::vec4> outputsVec4;
			std::vector<float> outputs;

			// calculates the cubic-spline interpolation
			glm::vec4 CubicSplineInterpolation(size_t index, float time, uint32_t stride);

			// translates the node
			void Translate(size_t index, float time, Node* node);

			// scales the node
			void Scale(size_t index, float time, Node* node);

			// rotates the node
			void Rotate(size_t index, float time, Node* node);
		};

		std::string name;
		std::vector<Channel> channels;
		std::vector<Sampler> samplers;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
	};
}

namespace Cosmos::Vulkan
{
	class VKMesh : public Mesh
	{
	public:

		struct LoaderInfo
		{
			uint32_t* indexBuffer;
			Vertex* vertexBuffer;
			size_t indexPos = 0;
			size_t vertexPos = 0;
		};

	public:

		//constructor
		VKMesh(Shared<VKRenderer> renderer);
	
		// destructor
		virtual ~VKMesh();
	
		// returns the mesh file name
		virtual inline std::string GetFilepath() const override { return mFilepath; }
	
		// returns if the mesh is fully loaded
		virtual inline bool IsLoaded() const override { return mLoaded; }
	
		// sets the render mode to wiredframe/fill
		virtual bool* GetWiredframe() override { return &mWiredframe; }
		
		// returns the vector of vertices of the mesh
		virtual std::vector<Vertex> GetVertices() const override { return mVertices; }

	public:
	
		// updates the mesh logic
		virtual void OnUpdate(float timestep) override;
	
		// draws the mesh
		virtual void OnRender(void* commandBuffer, glm::mat4& transform, uint32_t id) override;
	
		// loads the model from a filepath
		virtual void LoadFromFile(std::string filepath, float scale = 1.0f) override;

		// returns the mesh dimension
		virtual Dimension GetDimension() const override;

	public:

		// modifies the mesh material's colormap
		virtual void SetColormapTexture(std::string filepath) override;

		// returns the colormap texture used by the material's mesh
		virtual Shared<Texture2D> GetColormapTexture() override;

	private: // gltf related

		// updates an animation
		void UpdateAnimation(uint32_t index, float time);

		// draws a node, including it's children if any
		void DrawNode(GLTF::Node* node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
		
		// loads a gltf node
		void LoadNode(GLTF::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, LoaderInfo& loaderInfo, float globalscale = 1.0f);

		// returns usefull node properties
		void GetNodeProperties(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount);

		// load any material the mesh may have
		void LoadMaterials(tinygltf::Model& model);

		// load any animation the mesh may have
		void LoadAnimations(tinygltf::Model& gltfModel);

		// load mesh skins 
		void LoadSkins(tinygltf::Model& gltfModel);

		// returns a given node based on it's parent
		GLTF::Node* GetNode(GLTF::Node* parent, uint32_t index);

		// returns a given node based on it's index
		GLTF::Node* GetNodeFromIndex(uint32_t index);

	public: // renderer related

		// creates all renderer resources used by the mesh
		void CreateRendererResources(size_t vertexCount, size_t indexCount, LoaderInfo& loaderInfo);

		// creates the uniform buffers and setup descriptors
		void SetupDescriptors();
		
		// updates the descriptor sets
		void UpdateDescriptors();

	public: // size related

		// calculates the initial mesh dimension
		void CalculateMeshDimension();

		// calculates the bounding box of a node
		void CalculateBoundingBox(GLTF::Node* node, GLTF::Node* parent);

	private:

		Shared<VKRenderer> mRenderer;
		std::string mFilepath = {};
		bool mPicked = false;
		bool mLoaded = false;
		bool mWiredframe = false;
		Dimension mDimension;
		
		// mesh properties
		std::vector<Vertex> mVertices = {};

		// gpu data
		VkBuffer mVertexBuffer = VK_NULL_HANDLE;
		VmaAllocation mVertexMemory = VK_NULL_HANDLE;
		int mIndicesCount = 0;
		VkBuffer mIndexBuffer = VK_NULL_HANDLE;
		VmaAllocation mIndexMemory = VK_NULL_HANDLE;

		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> mDescriptorSets = {};

		// mesh data
		Material mMaterial;
		std::vector<GLTF::Node*> mNodes = {};
		std::vector<GLTF::Node*> mLinearNodes = {};
		std::vector<GLTF::Skin*> mSkins;
		std::vector<GLTF::Animation> mAnimations;
	};
}

#endif