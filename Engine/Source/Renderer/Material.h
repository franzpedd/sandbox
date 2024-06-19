#pragma once

#include "Texture.h"
#include "Util/Math.h"

namespace Cosmos
{
	class Material
	{
	public:

		enum AlphaChannel
		{
			OPAQUE,
			MASK,
			BLEND
		};

		struct BaseSpecification
		{
			// transparency
			AlphaChannel alphaChannel = OPAQUE;
			float alhpaCutoff = 1.0f;

			// base 
			bool doubleSided = false;
			glm::vec4 baseColorFactor = glm::vec4(1.0f);
			glm::vec4 emissiveFactor = glm::vec4(0.0f);
			Shared<Texture2D> baseColorTexture;
			Shared<Texture2D> normalTexture;
			Shared<Texture2D> occlusionTexture;
			Shared<Texture2D> emissiveTexture;
		};

		struct MetallicRoughnessWorkflow
		{
			float metallicFactor = 1.0f;
			float roughnessFactor = 1.0f;
			Shared<Texture2D> metallicRoughnessTexture;
		};

	public:

		// constructor
		Material() = default;

		// destructor
		~Material() = default;

	public:

		// returns a reference to the material's base specification
		inline BaseSpecification& GetBaseSpecificationRef() { return mBaseSpecification; }

		// returns a reference to the metallic roughness specification/workflow
		inline MetallicRoughnessWorkflow& GetMetallicRoughnessWorkflow() { return mMetallicRoughnessWorkflow; }

		// returns the descriptor set, currently it is a VkDescriptorSet
		inline void* GetDescriptorSet() { return mDescriptorSet; }

	private:

		BaseSpecification mBaseSpecification;
		MetallicRoughnessWorkflow mMetallicRoughnessWorkflow;
		void* mDescriptorSet = nullptr;
	};
}