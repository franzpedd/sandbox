#pragma once
#if defined COSMOS_RENDERER_VULKAN

// changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

#include "Physics/BoundingBox.h"
#include "Renderer/Material.h"
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

		// forward declaration
		struct Node;

		// helper, will become members when finished
		struct LoaderInfo
		{
			uint32_t* indexbuffer;
			size_t indexpos = 0;

			Vertex* vertexbuffer;
			size_t vertexpos = 0;
		};

		struct Primitive
		{
			uint32_t firstIndex;
			uint32_t indexCount;
			uint32_t vertexCount;
			Material& material;
			bool hasIndices;
			BoundingBox boundingBox;

			// constructor
			Primitive(Material& material, uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount);

			// sets a bounding box to the primitive
			void SetBoundingBox(glm::vec3 min, glm::vec3 max);
		};

		struct Skin
		{
			std::string name;
			Node* root = nullptr;
			std::vector<glm::mat4> inverseBindMatrices;
			std::vector<Node*> joints;
		};

		struct Internal
		{
			struct UniformBuffer
			{
				VkBuffer buffer;
				VmaAllocation allocation;
				VkDescriptorBufferInfo descriptor;
				VkDescriptorSet descriptorSet;
				void* mapped;
			};

			struct UniformBlock
			{
				glm::mat4 matrix;
				glm::mat4 jointMatrix[MAX_NUM_JOINTS] = {};
				uint32_t jointCount = 0;
			};

			Shared<Device> device;
			std::vector<Primitive*> primitives;
			BoundingBox boundingBox;
			BoundingBox axisAlignedBoundingBox;
			UniformBuffer uniformBuffer;
			UniformBlock uniformBlock;

			// constructor
			Internal(Shared<Device> device, glm::mat4 matrix);

			// destructor
			~Internal();

			// sets a bounding box for the mesh
			void SetBoundingBox(glm::vec3 min, glm::vec4 max);
		};

		struct Node
		{
			Node* parent = nullptr;
			uint32_t index = 0;
			std::vector<Node*> children;
			glm::mat4 matrix;
			std::string name;
			Internal* mesh = nullptr;
			Skin* skin = nullptr;
			int32_t skinIndex = -1;
			glm::vec3 translation = glm::vec3();
			glm::quat rotation = glm::quat();
			glm::vec3 scale = glm::vec3(1.0f);
			BoundingBox boundingVolumeHierarchy;
			BoundingBox axisAlignedBoundingBox;
			bool useCachedMatrix = false;
			glm::mat4 cachedLocalMatrix = glm::mat4(1.0f);
			glm::mat4 cachedMatrix = glm::mat4(1.0f);
			
			// constructor
			Node() = default;

			// destructor
			~Node();

			// returns the local node matrix
			glm::mat4 GetLocalMatrix();

			// returns the node matrix (it considers it's children)
			glm::mat4 GetMatrix();

			// updates the mesh 
			void OnUpdate();
		};

		struct Animation
		{
			struct Channel
			{
				enum PathType
				{
					TRANSLATION,
					ROTATION,
					SCALE
				};

				PathType path;
				Node* node;
				uint32_t samplerIndex;
			};

			struct Sampler
			{
				enum InterpolationType
				{
					LINEAR,
					STEP,
					CUBICSPLINE
				};

				InterpolationType interpolation;
				std::vector<float> inputs;
				std::vector<glm::vec4> outputsVec4;
				std::vector<float> outputs;

				// calculates the inteprolation of the cubic spline type
				glm::vec4 CubicSplineInterpolation(size_t index, float time, uint32_t stride);

				// calculates the translation of this sampler for the given node at a given time point depending on the interpolation type
				void Translate(Node* node, size_t index, float time);

				// calculates the scale of this sampler for the given node at a given time point depending on the interpolation type
				void Scale(Node* node, size_t index, float time);

				// calculates the rotation of this sampler for the given node at a given time point depending on the interpolation type
				void Rotate(Node* node, size_t index, float time);
			};

			std::string name;
			std::vector<Sampler> samplers;
			std::vector<Channel> channels;
			float start = std::numeric_limits<float>::max();
			float end = std::numeric_limits<float>::min();
		};

	public:

		// constructor
		VKMesh(Shared<VKRenderer> renderer);

		// destructor
		virtual ~VKMesh();

		// returns the mesh file name
		virtual inline std::string GetFilepath() const override { return mFilepath; }

		// returns if the mesh is fully loaded
		virtual bool IsLoaded() const override { return mLoaded; }

	public:

		// updates the mesh logic
		virtual void OnUpdate(float timestep, glm::mat4& transform) override;

		// draws the mesh
		virtual void OnRender() override;

		// loads the model from a filepath
		virtual void LoadFromFile(std::string filepath, float scale = 1.0f) override;

	private:

		// calculates the mesh dimensions
		void CalculateSceneDimensions();

		// calculates the bounding box of a node
		void CalculateBoundingBox(Node* node, Node* parent);

		// interprets the mesh node, reading it's vertex and index count
		void GetVertexAndIndicesCount(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount);

		// loads a mesh node
		void LoadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, LoaderInfo& loaderInfo, float globalscale);

		// finds a node by it's parent
		Node* FindNode(Node* parent, uint32_t index);

		// returns a node given it's index
		Node* NodeFromIndex(uint32_t index);

		// renders a node
		void DrawNode(Node* node, VkCommandBuffer commandBuffer);

	private:

		// loads meshe's material properties, as well as creating a default one if it doesn't have any
		void LoadMaterials(tinygltf::Model& model);

		// loads any animation the mesh may have
		void LoadAnimations(tinygltf::Model& model);

		// loads any skin the mesh may have
		void LoadSkins(tinygltf::Model& model);

		// creates the vulkan buffers and resources for the mesh
		void CreateRendererResources(LoaderInfo& loaderInfo, size_t vertexBufferSize, size_t indexBufferSize);

		// update the descriptor set, used whenever a texture resource gets modified/created
		void UpdateDescriptorSets();

	public:

		Shared<VKRenderer> mRenderer;
		std::string mFilepath = {};
		bool mLoaded = false;

		// scene dimensions
		glm::vec3 mDimensionMin = glm::vec3(FLT_MAX);
		glm::vec3 mDimensionMax = glm::vec3(-FLT_MAX);
		glm::mat4 mAABB;

		// gltf data
		std::vector<Node*> mNodes;
		std::vector<Node*> mLinearNodes;
		std::vector<Material> mMaterials;
		std::vector<Animation> mAnimations;
		std::vector<Skin*> mSkins;

		// vulkan resources
		VkBuffer mVertexBuffer = VK_NULL_HANDLE;
		VmaAllocation mVertexMemory = VK_NULL_HANDLE;
		VkBuffer mIndexBuffer = VK_NULL_HANDLE;
		VmaAllocation mIndexMemory = VK_NULL_HANDLE;

		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> mDescriptorSets = {};

		// camera's model, view and projection ubo
		std::vector<VkBuffer> mUniformBuffers;
		std::vector<VmaAllocation> mUniformBuffersMemory;
		std::vector<void*> mUniformBuffersMapped;
	};
}

#endif