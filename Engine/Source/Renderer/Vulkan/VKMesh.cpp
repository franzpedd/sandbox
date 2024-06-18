#include "epch.h"
#if defined COSMOS_RENDERER_VULKAN

#include "Renderer/Material.h"
#include "Device.h"
#include "VKMesh.h"
#include "VKTexture.h"
#include "VKRenderer.h"
#include "Util/Logger.h"

namespace Cosmos::Vulkan
{
	VKMesh::VKMesh(Shared<VKRenderer> renderer)
		: mRenderer(renderer)
	{
		
	}

	VKMesh::~VKMesh()
	{

	}

	void VKMesh::LoadFromFile(std::string filepath)
	{
		COSMOS_LOG(Logger::Todo, "Implement resource pooling");
		mFilepath = filepath;

		tinygltf::Model model;
		tinygltf::TinyGLTF context;

		std::string error;
		std::string warning;

		bool binary = false;
		size_t extpos = mFilepath.rfind('.', mFilepath.length());
		if (extpos != std::string::npos) {
			binary = (mFilepath.substr(extpos + 1, mFilepath.length() - extpos) == "glb");
		}

		bool fileLoaded = binary ? context.LoadBinaryFromFile(&model, &error, &warning, mFilepath.c_str()) : context.LoadASCIIFromFile(&model, &error, &warning, mFilepath.c_str());

		LoaderInfo loaderInfo = {};
		size_t vertexCount = 0;
		size_t indexCount = 0;

		if (!fileLoaded) {
			COSMOS_LOG(Logger::Error, "Failed to load %s, error: %s", mFilepath.c_str(), error.c_str());
			return;
		}

		else if (warning.size() > 0) {
			COSMOS_LOG(Logger::Warn, "Loaded %s with warnings: %s", mFilepath.c_str(), warning.c_str());
		}

		else {
			COSMOS_LOG(Logger::Trace, "Loaded %s successfuly", mFilepath.c_str());
		}

		// this is not finished in order to avoid linking a gltf file to a specific texture or material
		//LoadSamplers(model);
		//LoadTextures(model);
		//LoadMaterials(model);

		const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		// get vertex and index buffer sizes up-front
		for (size_t i = 0; i < scene.nodes.size(); i++) {
			GetVertexAndIndicesCount(model.nodes[scene.nodes[i]], model, vertexCount, indexCount);
		}

		loaderInfo.vertexbuffer = new Vertex[vertexCount];
		loaderInfo.indexbuffer = new uint32_t[indexCount];

		for (size_t i = 0; i < scene.nodes.size(); i++) {
			const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
			loadNode(nullptr, node, scene.nodes[i], gltfModel, loaderInfo, scale);
		}
		if (gltfModel.animations.size() > 0) {
			loadAnimations(gltfModel);
		}
		loadSkins(gltfModel);

	}

	void VKMesh::GetVertexAndIndicesCount(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount)
	{
		if (node.children.size() > 0) {
			for (size_t i = 0; i < node.children.size(); i++) {
				GetVertexAndIndicesCount(model.nodes[node.children[i]], model, vertexCount, indexCount);
			}
		}

		if (node.mesh > -1)	{
			const tinygltf::Mesh mesh = model.meshes[node.mesh];

			for (size_t i = 0; i < mesh.primitives.size(); i++) {
				auto primitive = mesh.primitives[i];
				vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
				if (primitive.indices > -1) {
					indexCount += model.accessors[primitive.indices].count;
				}
			}
		}
	}

	void VKMesh::LoadSamplers(tinygltf::Model& model)
	{
		for (tinygltf::Sampler& sample : model.samplers)
		{
			TextureSampler sampler = {};
			sampler.min = TextureSampler::FilterMode(sample.minFilter);
			sampler.mag = TextureSampler::FilterMode(sample.magFilter);
			sampler.u = TextureSampler::WrapMode(sample.wrapS);
			sampler.v = TextureSampler::WrapMode(sample.wrapT);
			sampler.w = sampler.v;
			mSamplers.push_back(sampler);
		}
	}

	void VKMesh::LoadTextures(tinygltf::Model& model)
	{
		//for (tinygltf::Texture& texture : model.textures)
		//{
		//	tinygltf::Image image = model.images[texture.source];
		//	TextureSampler sampler;
		//
		//	if (texture.sampler == -1) {
		//		sampler.mag = TextureSampler::Filter::FILTER_LINEAR;
		//		sampler.min = TextureSampler::Filter::FILTER_LINEAR;
		//		sampler.u = TextureSampler::AddressMode::ADDRESS_MODE_REPEAT;
		//		sampler.v = TextureSampler::AddressMode::ADDRESS_MODE_REPEAT;
		//		sampler.w = TextureSampler::AddressMode::ADDRESS_MODE_REPEAT;
		//	}
		//
		//	else {
		//		sampler = mSamplers[texture.sampler];
		//	}
		//
		//	// load texture
		//	unsigned char* buffer = nullptr;
		//	VkDeviceSize bufferSize = 0;
		//	bool deleteBuffer = false;
		//
		//	// most devices don't support RGB only on Vulkan so convert if necessary
		//	if (image.component == 3) 
		//	{
		//		bufferSize = image.width * image.height * 4;
		//		buffer = new unsigned char[bufferSize];
		//		unsigned char* rgba = buffer;
		//		unsigned char* rgb = &image.image[0];
		//
		//		for (int32_t i = 0; i < image.width * image.height; ++i) {
		//			for (int32_t j = 0; j < 3; ++j) {
		//				rgba[j] = rgb[j];
		//			}
		//			rgba += 4;
		//			rgb += 3;
		//		}
		//		deleteBuffer = true;
		//	}
		//
		//	else {
		//		buffer = &image.image[0];
		//		bufferSize = image.image.size();
		//	}
		//
		//	int32_t width = image.width;
		//	int32_t height = image.height;
		//	uint32_t mipLevels = (uint32_t)(std::floor(std::log2(std::max(width, height))) + 1.0);
		//
		//	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		//	VkFormatProperties formatProperties;
		//	vkGetPhysicalDeviceFormatProperties(mRenderer->GetDevice()->GetPhysicalDevice(), format, &formatProperties);
		//	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
		//	assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
		//
		//	VkBuffer stagingBuffer;
		//	VmaAllocation stagingAllocation;
		//
		//	mRenderer->GetDevice()->CreateBuffer
		//	(
		//		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//		bufferSize,
		//		&stagingBuffer,
		//		&stagingAllocation
		//	);
		//
		//	if (buffer != nullptr)
		//	{
		//		void* data = nullptr;
		//		vmaMapMemory(mRenderer->GetDevice()->GetAllocator(), stagingAllocation, &data);
		//		memcpy(data, buffer, (size_t)bufferSize);
		//		vmaUnmapMemory(mRenderer->GetDevice()->GetAllocator(), stagingAllocation);
		//
		//		if (deleteBuffer) {
		//			delete[] buffer;
		//		}
		//	}
		//}
	}

	void VKMesh::LoadMaterials(tinygltf::Model& model)
	{
		for (tinygltf::Material& mat : model.materials)
		{
			Material material = {};
			material.doubleSided = mat.doubleSided;

			if (mat.values.find("roughnessFactor") != mat.values.end()) {
				material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
			}

			if (mat.values.find("metallicFactor") != mat.values.end()) {
				material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
			}

			if (mat.values.find("baseColorFactor") != mat.values.end()) {
				material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
			}

			if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
				material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
			}

			if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
				material.texCoordSets.emissive = mat.additionalValues["emissiveTexture"].TextureTexCoord();
			}

			if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
				material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
			}

			if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
				tinygltf::Parameter param = mat.additionalValues["alphaMode"];
				if (param.string_value == "BLEND") {
					material.alphaMode = Material::ALPHAMODE_BLEND;
				}

				if (param.string_value == "MASK") {
					material.alphaCutoff = 0.5f;
					material.alphaMode = Material::ALPHAMODE_MASK;
				}
			}

			if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
				material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
			}

			if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
				material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
			}

			material.index = static_cast<uint32_t>(mMaterials.size());
			mMaterials.push_back(material);
		}

		// push a default material at the end of the list for meshes with no material assigned
		mMaterials.push_back(Material());
	}
}

#endif