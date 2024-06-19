#pragma once
#if defined COSMOS_RENDERER_VULKAN

// Changing this value here also requires changing it in the vertex shader
#define MAX_NUM_JOINTS 128u

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

		struct MeshInternal
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
			MeshInternal(Shared<Device> device, glm::mat4 matrix);

			// destructor
			~MeshInternal();

			// sets a bounding box for the mesh
			void SetBoundingBox(glm::vec3 min, glm::vec4 max);
		};

		struct Node
		{
			Node* parent;
			uint32_t index;
			std::vector<Node*> children;
			glm::mat4 matrix;
			std::string name;
			MeshInternal* mesh;
			Skin* skin;
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

				// translates
				void Translate(Node* node, size_t index, float time);

				// rotates
				void Scale(Node* node, size_t index, float time);

				// rotates
				void Rotate(Node* node, size_t index, float time);


			};
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