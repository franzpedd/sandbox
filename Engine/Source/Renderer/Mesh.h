#pragma once

#include "Texture.h"
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

	class Material
	{
	public:

		enum AlphaMode
		{
			ALPHAMODE_OPAQUE,
			ALPHAMODE_MASK,
			ALPHAMODE_BLEND
		};

		AlphaMode alphaMode = ALPHAMODE_OPAQUE;
		float alphaCutoff = 1.0f;
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		glm::vec4 emissiveFactor = glm::vec4(0.0f);
		Shared<Texture2D> baseColorTexture;
		Shared<Texture2D> metallicRoughnessTexture;
		Shared<Texture2D> normalTexture;
		Shared<Texture2D> occlusionTexture;
		Shared<Texture2D> emissiveTexture;
		bool doubleSided = false;

		struct TexCoordSets
		{
			uint8_t baseColor = 0;
			uint8_t metallicRoughness = 0;
			uint8_t specularGlossiness = 0;
			uint8_t normal = 0;
			uint8_t occlusion = 0;
			uint8_t emissive = 0;
		} texCoordSets;

		struct Extension
		{
			Shared<Texture2D> specularGlossinessTexture;
			Shared<Texture2D> diffuseTexture;
			glm::vec4 diffuseFactor = glm::vec4(1.0f);
			glm::vec3 specularFactor = glm::vec3(0.0f);
		} extension;

		struct PbrWorkflows
		{
			bool metallicRoughness = true;
			bool specularGlossiness = false;
		} pbrWorkflows;

		void* descriptorSet = nullptr;
		int index = 0;
		bool unlit = false;
		float emissiveStrength = 1.0f;
	};
}