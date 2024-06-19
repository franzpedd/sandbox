#include "epch.h"
#if defined COSMOS_RENDERER_VULKAN

#include "Device.h"
#include "Renderpass.h"
#include "VKMesh.h"
#include "VKTexture.h"
#include "VKRenderer.h"
#include "Util/Logger.h"

namespace Cosmos::Vulkan
{
	VKMesh::Primitive::Primitive(Material& material, uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount)
		: material(material), firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount)
	{
		hasIndices = indexCount > 0;
	}

	void VKMesh::Primitive::SetBoundingBox(glm::vec3 min, glm::vec3 max)
	{
		boundingBox.GetMaxRef() = max;
		boundingBox.GetMinRef() = min;
		boundingBox.SetValid(true);
	}

	VKMesh::Internal::Internal(Shared<Device> device, glm::mat4 matrix)
		: device(device)
	{
		uniformBlock.matrix = matrix;

		device->CreateBuffer
		(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uniformBlock),
			&uniformBuffer.buffer,
			&uniformBuffer.allocation,
			&uniformBlock
		);

		vmaMapMemory(device->GetAllocator(), uniformBuffer.allocation, &uniformBuffer.mapped);
	}

	VKMesh::Internal::~Internal()
	{
		vmaDestroyBuffer(device->GetAllocator(), uniformBuffer.buffer, uniformBuffer.allocation);
		vmaFreeMemory(device->GetAllocator(), uniformBuffer.allocation);

		for (Primitive* prim : primitives)
		{
			delete prim;
		}
	}

	void VKMesh::Internal::SetBoundingBox(glm::vec3 min, glm::vec4 max)
	{
		boundingBox.GetMinRef() = min;
		boundingBox.GetMaxRef() = max;
		boundingBox.SetValid(true);
	}

	VKMesh::Node::~Node()
	{
		if (mesh) 
			delete mesh;

		for (auto& child : children) 
			delete child;
	}

	glm::mat4 VKMesh::Node::GetLocalMatrix()
	{
		if (!useCachedMatrix) {
			cachedLocalMatrix = glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
		}

		return cachedLocalMatrix;
	}

	glm::mat4 VKMesh::Node::GetMatrix()
	{
		if (!useCachedMatrix) {
			glm::mat4 finalMatrix = GetLocalMatrix();
			Node* aux = parent;

			while (aux != nullptr) {
				finalMatrix = aux->GetLocalMatrix() * finalMatrix;
				aux = aux->parent;
			}

			cachedMatrix = finalMatrix;
			useCachedMatrix = true;

			return finalMatrix;
		}

		return cachedMatrix;
	}

	void VKMesh::Node::OnUpdate()
	{
		useCachedMatrix = false;

		if (mesh) {
			glm::mat4 mat = GetLocalMatrix();

			if (skin) {
				mesh->uniformBlock.matrix = mat;

				// update joint matrices
				glm::mat4 inverseTransform = glm::inverse(mat);
				size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);

				for (size_t i = 0; i < numJoints; i++) {
					Node* jointNode = skin->joints[i];
					glm::mat4 jointMat = jointNode->GetMatrix() * skin->inverseBindMatrices[i];
					jointMat = inverseTransform * jointMat;
					mesh->uniformBlock.jointMatrix[i] = jointMat;
				}

				mesh->uniformBlock.jointCount = (uint32_t)numJoints;
				memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
			}

			else {
				memcpy(mesh->uniformBuffer.mapped, &mat, sizeof(glm::mat4));
			}
		}

		for (auto& child : children) {
			child->OnUpdate();
		}
	}

	glm::vec4 VKMesh::Animation::Sampler::CubicSplineInterpolation(size_t index, float time, uint32_t stride)
	{
		float delta = inputs[index + 1] - inputs[index];
		float t = (time - inputs[index]) / delta;
		const size_t current = index * stride * 3;
		const size_t next = (index + 1) * stride * 3;
		const size_t A = 0;
		const size_t V = stride * 1;
		const size_t B = stride * 2;

		float t2 = powf(t, 2);
		float t3 = powf(t, 3);
		glm::vec4 pt{ 0.0f };

		for (uint32_t i = 0; i < stride; i++) {
			float p0 = outputs[current + i + V];			// starting point at t = 0
			float m0 = delta * outputs[current + i + A];	// scaled starting tangent at t = 0
			float p1 = outputs[next + i + V];				// ending point at t = 1
			float m1 = delta * outputs[next + i + B];		// scaled ending tangent at t = 1
			pt[i] = ((2.f * t3 - 3.f * t2 + 1.f) * p0) + ((t3 - 2.f * t2 + t) * m0) + ((-2.f * t3 + 3.f * t2) * p1) + ((t3 - t2) * m0);
		}

		return pt;
	}

	void VKMesh::Animation::Sampler::Translate(Node* node, size_t index, float time)
	{
		switch (interpolation)
		{
			case Sampler::InterpolationType::LINEAR:
			{
				float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
				node->translation = glm::mix(outputsVec4[index], outputsVec4[index + 1], u);
				break;
			}

			case Sampler::InterpolationType::STEP:
			{
				node->translation = outputsVec4[index];
				break;
			}

			case Sampler::InterpolationType::CUBICSPLINE:
			{
				node->translation = CubicSplineInterpolation(index, time, 3);
				break;
			}
		}
	}

	void VKMesh::Animation::Sampler::Scale(Node* node, size_t index, float time)
	{
		switch (interpolation)
		{
			case Sampler::InterpolationType::LINEAR:
			{
				float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
				node->scale = glm::mix(outputsVec4[index], outputsVec4[index + 1], u);
				break;
			}

			case Sampler::InterpolationType::STEP:
			{
				node->scale = outputsVec4[index];
				break;
			}

			case Sampler::InterpolationType::CUBICSPLINE:
			{
				node->scale = CubicSplineInterpolation(index, time, 3);
				break;
			}
		}
	}

	void VKMesh::Animation::Sampler::Rotate(Node* node, size_t index, float time)
	{
		switch (interpolation)
		{
			case Sampler::InterpolationType::LINEAR:
			{
				float u = std::max(0.0f, time - inputs[index]) / (inputs[index + 1] - inputs[index]);
				glm::quat q1;
				q1.x = outputsVec4[index].x;
				q1.y = outputsVec4[index].y;
				q1.z = outputsVec4[index].z;
				q1.w = outputsVec4[index].w;
				glm::quat q2;
				q2.x = outputsVec4[index + 1].x;
				q2.y = outputsVec4[index + 1].y;
				q2.z = outputsVec4[index + 1].z;
				q2.w = outputsVec4[index + 1].w;
				node->rotation = glm::normalize(glm::slerp(q1, q2, u));
				break;
			}

			case Sampler::InterpolationType::STEP:
			{
				glm::quat q1;
				q1.x = outputsVec4[index].x;
				q1.y = outputsVec4[index].y;
				q1.z = outputsVec4[index].z;
				q1.w = outputsVec4[index].w;
				node->rotation = q1;
				break;
			}

			case Sampler::InterpolationType::CUBICSPLINE:
			{
				glm::vec4 rot = CubicSplineInterpolation(index, time, 4);
				glm::quat q;
				q.x = rot.x;
				q.y = rot.y;
				q.z = rot.z;
				q.w = rot.w;
				node->rotation = glm::normalize(q);
				break;
			}
		}
	}

	VKMesh::VKMesh(Shared<VKRenderer> renderer)
		: mRenderer(renderer)
	{
		
	}

	VKMesh::~VKMesh()
	{
		if (mVertexBuffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), mVertexBuffer, mVertexMemory);
		}

		if (mIndexBuffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), mIndexBuffer, mIndexMemory);
		}

		for (auto node : mNodes)
			delete node;

		for (auto skin : mSkins)
			delete skin;

		mNodes.resize(0);
		mLinearNodes.resize(0);
		mSkins.resize(0);
		mMaterials.resize(0);
		mAnimations.resize(0);
	}

	void VKMesh::OnUpdate(float timestep)
	{
		if (mAnimations.empty())
			return;

		COSMOS_LOG(Logger::Todo, "Animation system is not fully implemented");

		uint32_t index = 0;
		if (index > static_cast<uint32_t>(mAnimations.size()) - 1)
		{
			COSMOS_LOG(Logger::Warn, "No animation with index %d", index);
			return;
		}

		Animation& animation = mAnimations[index];

		bool updated = false;
		for (auto& channel : animation.channels)
		{
			Animation::Sampler& sampler = animation.samplers[channel.samplerIndex];

			if (sampler.inputs.size() > sampler.outputsVec4.size())
				continue;

			for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
			{
				if ((timestep >= sampler.inputs[i]) && (timestep <= sampler.inputs[i + 1]))
				{
					float u = std::max(0.0f, timestep - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
					if (u <= 1.0f)
					{
						switch (channel.path)
						{
							case Animation::Channel::PathType::TRANSLATION:
							{
								sampler.Translate(channel.node, i, timestep);
								break;
							}
								
							case Animation::Channel::PathType::SCALE:
							{
								sampler.Scale(channel.node, i, timestep);
								break;
							}
								
							case Animation::Channel::PathType::ROTATION:
							{
								sampler.Rotate(channel.node, i, timestep);
								break;
							}
						}
						updated = true;
					}
				}
			}
		}

		if (updated)
		{
			for (auto& node : mNodes)
			{
				node->OnUpdate();
			}
		}
	}

	void VKMesh::OnRender()
	{
		uint32_t currentFrame = mRenderer->GetCurrentFrame();
		VkDeviceSize offsets[] = { 0 };
		VkCommandBuffer cmdBuffer = mRenderer->GetRenderpassManager()->GetMainRenderpass()->GetSpecificationRef().commandBuffers[currentFrame];

		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mVertexBuffer, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, mIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		for (auto& node : mNodes)
			DrawNode(node, cmdBuffer);
	}

	void VKMesh::LoadFromFile(std::string filepath, float scale)
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

		LoadMaterials(model);

		const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		// get vertex and index buffer sizes up-front
		for (size_t i = 0; i < scene.nodes.size(); i++) {
			GetVertexAndIndicesCount(model.nodes[scene.nodes[i]], model, vertexCount, indexCount);
		}

		loaderInfo.vertexbuffer = new Vertex[vertexCount];
		loaderInfo.indexbuffer = new uint32_t[indexCount];

		for (size_t i = 0; i < scene.nodes.size(); i++) {
			const tinygltf::Node node = model.nodes[scene.nodes[i]];
			LoadNode(nullptr, node, scene.nodes[i], model, loaderInfo, scale);
		}

		if (model.animations.size() > 0) {
			LoadAnimations(model);
		}

		LoadSkins(model);

		for (auto node : mLinearNodes)
		{
			// assign skins
			if (node->skinIndex > -1)
			{
				node->skin = mSkins[node->skinIndex];
			}

			// set to initial pose
			if (node->mesh)
			{
				node->OnUpdate();
			}
		}

		size_t vSize = vertexCount * sizeof(Vertex);
		size_t iSize = indexCount * sizeof(uint32_t);

		CreateRendererResources(loaderInfo, vSize, iSize);
		CalculateSceneDimensions();
	}

	void VKMesh::CalculateSceneDimensions()
	{
		// calculate bounding volume hierarchy for all nodes in the scene
		for (auto node : mLinearNodes)
			CalculateBoundingBox(node, nullptr);

		mDimensionMin = glm::vec3(FLT_MAX);
		mDimensionMax = glm::vec3(-FLT_MAX);

		for (auto node : mLinearNodes)
		{
			if (node->boundingVolumeHierarchy.IsValid())
			{
				mDimensionMin = glm::min(mDimensionMin, node->boundingVolumeHierarchy.GetMinRef());
				mDimensionMax = glm::max(mDimensionMax, node->boundingVolumeHierarchy.GetMaxRef());
			}
		}

		// calculate scene aabb
		mAABB = glm::scale(glm::mat4(1.0f), glm::vec3(mDimensionMax[0] - mDimensionMin[0], mDimensionMax[1] - mDimensionMin[1], mDimensionMax[2] - mDimensionMin[2]));
		mAABB[3][0] = mDimensionMin[0];
		mAABB[3][1] = mDimensionMin[1];
		mAABB[3][2] = mDimensionMin[2];
	}

	void VKMesh::CalculateBoundingBox(Node* node, Node* parent)
	{
		BoundingBox parentBvh = parent ? parent->boundingVolumeHierarchy : BoundingBox(mDimensionMin, mDimensionMax);

		if (node->mesh)
		{
			if (node->mesh->boundingBox.IsValid())
			{
				node->axisAlignedBoundingBox = node->mesh->boundingBox.GetAABB(node->GetMatrix());

				if (node->children.size() == 0)
				{
					node->boundingVolumeHierarchy.GetMinRef() = node->axisAlignedBoundingBox.GetMinRef();
					node->boundingVolumeHierarchy.GetMaxRef() = node->axisAlignedBoundingBox.GetMaxRef();
					node->boundingVolumeHierarchy.SetValid(true);
				}
			}
		}

		parentBvh.GetMinRef() = glm::min(parentBvh.GetMinRef(), node->boundingVolumeHierarchy.GetMinRef());
		parentBvh.GetMaxRef() = glm::min(parentBvh.GetMaxRef(), node->boundingVolumeHierarchy.GetMaxRef());

		for (auto& child : node->children)
		{
			CalculateBoundingBox(child, node);
		}
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

	void VKMesh::LoadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, LoaderInfo& loaderInfo, float globalscale)
	{
		Node* newNode = new Node();
		newNode->index = nodeIndex;
		newNode->parent = parent;
		newNode->name = node.name;
		newNode->skinIndex = node.skin;
		newNode->matrix = glm::mat4(1.0f);

		// generate local node matrix
		glm::vec3 translation = glm::vec3(0.0f);
		if (node.translation.size() == 3) {
			translation = glm::make_vec3(node.translation.data());
			newNode->translation = translation;
		}

		glm::mat4 rotation = glm::mat4(1.0f);
		if (node.rotation.size() == 4) {
			glm::quat q = glm::make_quat(node.rotation.data());
			newNode->rotation = glm::mat4(q);
		}

		glm::vec3 scale = glm::vec3(1.0f);
		if (node.scale.size() == 3) {
			scale = glm::make_vec3(node.scale.data());
			newNode->scale = scale;
		}

		if (node.matrix.size() == 16) {
			newNode->matrix = glm::make_mat4x4(node.matrix.data());
		};

		// node has children, must load them
		if (node.children.size() > 0) {
			for (size_t i = 0; i < node.children.size(); i++) {
				LoadNode(newNode, model.nodes[node.children[i]], node.children[i], model, loaderInfo, globalscale);
			}
		}

		// node contains mesh data
		if (node.mesh > -1) {
			const tinygltf::Mesh mesh = model.meshes[node.mesh];
			Internal* newMesh = new Internal(mRenderer->GetDevice(), newNode->matrix);

			for (size_t j = 0; j < mesh.primitives.size(); j++) {
				const tinygltf::Primitive& primitive = mesh.primitives[j];
				uint32_t vertexStart = static_cast<uint32_t>(loaderInfo.vertexpos);
				uint32_t indexStart = static_cast<uint32_t>(loaderInfo.indexpos);
				uint32_t indexCount = 0;
				uint32_t vertexCount = 0;
				glm::vec3 posMin = {};
				glm::vec3 posMax = {};
				bool hasSkin = false;
				bool hasIndices = primitive.indices > -1;

				// vertices
				{
					const float* bufferPos = nullptr;
					const float* bufferNormals = nullptr;
					const float* bufferTexCoordSet0 = nullptr;
					const float* bufferTexCoordSet1 = nullptr;
					const float* bufferColorSet0 = nullptr;
					const void* bufferJoints = nullptr;
					const float* bufferWeights = nullptr;

					int posByteStride;
					int normByteStride;
					int uv0ByteStride;
					int uv1ByteStride;
					int color0ByteStride;
					int jointByteStride;
					int weightByteStride;

					int jointComponentType;

					// position attribute is required
					COSMOS_ASSERT(primitive.attributes.find("POSITION") != primitive.attributes.end(), "Mesh doesn't contain position info");

					const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
					bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
					posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
					posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
					vertexCount = static_cast<uint32_t>(posAccessor.count);
					posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

					// normal
					if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
						const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
						const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
						bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
						normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
					}

					// uv0
					if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
						const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
						const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
						bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
						uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
					}

					// uv1
					if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
						const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
						const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
						bufferTexCoordSet1 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
						uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
					}

					// vertex color
					if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
						const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
						const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
						bufferColorSet0 = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						color0ByteStride = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
					}

					// skinning joints
					if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
						const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
						const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
						bufferJoints = &(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
						jointComponentType = jointAccessor.componentType;
						jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointComponentType)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
					}

					// skinning weight
					if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
						const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
						const tinygltf::BufferView& weightView = model.bufferViews[weightAccessor.bufferView];
						bufferWeights = reinterpret_cast<const float*>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
						weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
					}

					hasSkin = (bufferJoints && bufferWeights);

					for (size_t v = 0; v < posAccessor.count; v++) {
						Vertex& vert = loaderInfo.vertexbuffer[loaderInfo.vertexpos];
						vert.position = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
						vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
						vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
						vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);
						vert.color = bufferColorSet0 ? glm::make_vec4(&bufferColorSet0[v * color0ByteStride]) : glm::vec4(1.0f);

						if (hasSkin)
						{
							switch (jointComponentType)
							{
								case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: 
								{
									const uint16_t* buf = static_cast<const uint16_t*>(bufferJoints);
									vert.joint = glm::uvec4(glm::make_vec4(&buf[v * jointByteStride]));
									break;
								}

								case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: 
								{
									const uint8_t* buf = static_cast<const uint8_t*>(bufferJoints);
									vert.joint = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
									break;
								}

								default:
								{
									COSMOS_LOG(Logger::Error, "Joint component type %d is not supported, removing skin", jointComponentType);
									vert.joint = glm::vec4(0.0f);
									break;
								}
							}
						}

						else
						{
							vert.joint = glm::vec4(0.0f);
						}

						vert.weight = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);

						// fix for all zero weights
						if (glm::length(vert.weight) == 0.0f) {
							vert.weight = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
						}

						loaderInfo.vertexpos++;
					}
				}

				// indices
				if (hasIndices)
				{
					const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

					indexCount = static_cast<uint32_t>(accessor.count);
					const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

					switch (accessor.componentType)
					{
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
						{
							const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++) {
								loaderInfo.indexbuffer[loaderInfo.indexpos] = buf[index] + vertexStart;
								loaderInfo.indexpos++;
							}
							break;
						}

						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
						{
							const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++) {
								loaderInfo.indexbuffer[loaderInfo.indexpos] = buf[index] + vertexStart;
								loaderInfo.indexpos++;
							}

							break;
						}

						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
						{
							const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++) {
								loaderInfo.indexbuffer[loaderInfo.indexpos] = buf[index] + vertexStart;
								loaderInfo.indexpos++;
							}

							break;
						}

						default:
						{
							COSMOS_LOG(Logger::Error, "Index component type %d is not supported", accessor.componentType);
							return;
						}
					}
				}

				Primitive* newPrimitive = new Primitive(primitive.material > -1 ? mMaterials[primitive.material] : mMaterials.back(), indexStart, indexCount, vertexCount);
				newPrimitive->SetBoundingBox(posMin, posMax);
				newMesh->primitives.push_back(newPrimitive);
			}

			// Mesh BB from BBs of primitives
			for (auto p : newMesh->primitives)
			{
				if (p->boundingBox.IsValid() && !newMesh->boundingBox.IsValid()) {
					newMesh->boundingBox = p->boundingBox;
					newMesh->boundingBox.SetValid(true);
				}
				newMesh->boundingBox.GetMinRef() = glm::min(newMesh->boundingBox.GetMinRef(), p->boundingBox.GetMinRef());
				newMesh->boundingBox.GetMaxRef() = glm::max(newMesh->boundingBox.GetMaxRef(), p->boundingBox.GetMaxRef());
			}
			newNode->mesh = newMesh;
		}

		if (parent) {
			parent->children.push_back(newNode);
		}

		else {
			mNodes.push_back(newNode);
		}

		mLinearNodes.push_back(newNode);
	}

	VKMesh::Node* VKMesh::FindNode(Node* parent, uint32_t index)
	{
		Node* found = nullptr;

		if (parent->index == index)	return parent;

		for (auto& child : parent->children) {
			found = FindNode(child, index);

			if (found) break;
		}

		return found;
	}

	VKMesh::Node* VKMesh::NodeFromIndex(uint32_t index)
	{
		Node* found = nullptr;

		for (auto& node : mNodes)
		{
			found = FindNode(node, index);

			if (found) break;
		}

		return found;
	}

	void VKMesh::DrawNode(Node* node, VkCommandBuffer commandBuffer)
	{
		if (node->mesh)
		{
			for (Primitive* primitive : node->mesh->primitives)
				vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
		}

		for (auto& child : node->children)
			DrawNode(child, commandBuffer);
	}

	void VKMesh::LoadMaterials(tinygltf::Model& model)
	{
		for (tinygltf::Material& mat : model.materials)
		{
			Material material = {};
			material.GetSpecificationRef().doubleSided = mat.doubleSided;

			if (mat.values.find("roughnessFactor") != mat.values.end()) {
				material.GetMetallicRoughnessWorkflow().roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
			}

			if (mat.values.find("metallicFactor") != mat.values.end()) {
				material.GetMetallicRoughnessWorkflow().metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
			}

			if (mat.values.find("baseColorFactor") != mat.values.end()) {
				material.GetSpecificationRef().baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
			}

			if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
				tinygltf::Parameter param = mat.additionalValues["alphaMode"];
				if (param.string_value == "BLEND") {
					material.GetSpecificationRef().alphaChannel = Material::AlphaChannel::BLEND;
				}

				if (param.string_value == "MASK") {
					material.GetSpecificationRef().alhpaCutoff = 0.5f;
					material.GetSpecificationRef().alphaChannel = Material::AlphaChannel::MASK;
				}
			}

			if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
				material.GetSpecificationRef().alhpaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
			}

			if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
				material.GetSpecificationRef().emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
			}

			material.GetSpecificationRef().index = static_cast<uint32_t>(mMaterials.size());
			mMaterials.push_back(material);
		}

		// push a default material at the end of the list for meshes with no material assigned
		mMaterials.push_back(Material());
	}

	void VKMesh::LoadAnimations(tinygltf::Model& model)
	{
		for (tinygltf::Animation& anim : model.animations) {
			Animation animation = {};
			animation.name = anim.name;

			if (anim.name.empty()) {
				animation.name = std::to_string(mAnimations.size());
			}

			// read samplers
			for (auto& samp : anim.samplers) {
				Animation::Sampler sampler = {};

				if (samp.interpolation == "LINEAR") {
					sampler.interpolation = Animation::Sampler::InterpolationType::LINEAR;
				}

				if (samp.interpolation == "STEP") {
					sampler.interpolation = Animation::Sampler::InterpolationType::STEP;
				}

				if (samp.interpolation == "CUBICSPLINE") {
					sampler.interpolation = Animation::Sampler::InterpolationType::CUBICSPLINE;
				}

				// read sampler input time values
				const tinygltf::Accessor& accessorInput = model.accessors[samp.input];
				const tinygltf::BufferView& bufferViewInput = model.bufferViews[accessorInput.bufferView];
				const tinygltf::Buffer& bufferInput = model.buffers[bufferViewInput.buffer];

				assert(accessorInput.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				const void* dataInputPtr = &bufferInput.data[accessorInput.byteOffset + bufferViewInput.byteOffset];
				const float* buf = static_cast<const float*>(dataInputPtr);
				for (size_t index = 0; index < accessorInput.count; index++) {
					sampler.inputs.push_back(buf[index]);
				}

				for (auto input : sampler.inputs) {
					if (input < animation.start) {
						animation.start = input;
					}

					if (input > animation.end) {
						animation.end = input;
					}
				}

				// read sampler output T/R/S values 
				const tinygltf::Accessor& accessorOutput = model.accessors[samp.output];
				const tinygltf::BufferView& bufferViewOutput = model.bufferViews[accessorOutput.bufferView];
				const tinygltf::Buffer& bufferOutput = model.buffers[bufferViewOutput.buffer];

				COSMOS_ASSERT(accessorOutput.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT, "Animation output valud component type must be float");

				const void* dataOutputPtr = &bufferOutput.data[accessorOutput.byteOffset + bufferViewOutput.byteOffset];

				switch (accessorOutput.type) {
					case TINYGLTF_TYPE_VEC3: {
						const glm::vec3* buf = static_cast<const glm::vec3*>(dataOutputPtr);
						for (size_t index = 0; index < accessorOutput.count; index++) {
							sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
							sampler.outputs.push_back(buf[index][0]);
							sampler.outputs.push_back(buf[index][1]);
							sampler.outputs.push_back(buf[index][2]);
						}
						break;
					}
					
					case TINYGLTF_TYPE_VEC4: {
						const glm::vec4* buf = static_cast<const glm::vec4*>(dataOutputPtr);
						for (size_t index = 0; index < accessorOutput.count; index++) {
							sampler.outputsVec4.push_back(buf[index]);
							sampler.outputs.push_back(buf[index][0]);
							sampler.outputs.push_back(buf[index][1]);
							sampler.outputs.push_back(buf[index][2]);
							sampler.outputs.push_back(buf[index][3]);
						}
						break;
					}

					default: {
						COSMOS_LOG(Logger::Error, "Unknown output sampler animation acessor type");
						break;
					}
				}

				animation.samplers.push_back(sampler);
			}

			// read channels
			for (auto& source : anim.channels) {
				Animation::Channel channel = {};

				if (source.target_path == "rotation") {
					channel.path = Animation::Channel::PathType::ROTATION;
				}

				if (source.target_path == "translation") {
					channel.path = Animation::Channel::PathType::TRANSLATION;
				}

				if (source.target_path == "scale") {
					channel.path = Animation::Channel::PathType::SCALE;
				}

				if (source.target_path == "weights") {
					COSMOS_LOG(Logger::Todo, "Animation currently doesn't supports weights, skipping");
					continue;
				}

				channel.samplerIndex = source.sampler;
				channel.node = NodeFromIndex(source.target_node);

				if (!channel.node) continue;

				animation.channels.push_back(channel);
			}

			mAnimations.push_back(animation);
		}
	}

	void VKMesh::LoadSkins(tinygltf::Model& model)
	{
		for (tinygltf::Skin& source : model.skins) {
			Skin* newSkin = new Skin();
			newSkin->name = source.name;

			// find skeleton root node
			if (source.skeleton > -1)  newSkin->root = NodeFromIndex(source.skeleton);

			// find joint nodes
			for (int jointIndex : source.joints)
			{
				Node* node = NodeFromIndex(jointIndex);
				if (node) newSkin->joints.push_back(NodeFromIndex(jointIndex));
			}

			// get inverse bind matrices from buffer
			if (source.inverseBindMatrices > -1)
			{
				const tinygltf::Accessor& accessor = model.accessors[source.inverseBindMatrices];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
				newSkin->inverseBindMatrices.resize(accessor.count);
				memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
			}

			mSkins.push_back(newSkin);
		}
	}

	void VKMesh::CreateRendererResources(LoaderInfo& loaderInfo, size_t vertexBufferSize, size_t indexBufferSize)
	{
		VkBuffer vertexStagingBuffer;
		VmaAllocation vertexStagingMemory;

		VkBuffer IndexStagingBuffer;
		VmaAllocation IndexStagingMemory;

		// create staging buffers
		COSMOS_ASSERT
		(
			mRenderer->GetDevice()->CreateBuffer
			(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				vertexBufferSize,
				&vertexStagingBuffer,
				&vertexStagingMemory,
				loaderInfo.vertexbuffer
			) == VK_SUCCESS, "Failed to create vertex staging buffer for mesh"
		);

		if (indexBufferSize > 0)
		{
			COSMOS_ASSERT
			(
				mRenderer->GetDevice()->CreateBuffer
				(
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					indexBufferSize,
					&IndexStagingBuffer,
					&IndexStagingMemory,
					loaderInfo.indexbuffer
				) == VK_SUCCESS, "Failed to create index staging buffer for mesh"
			);
		}

		// create device local buffers
		COSMOS_ASSERT
		(
			mRenderer->GetDevice()->CreateBuffer
			(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				vertexBufferSize,
				&mVertexBuffer,
				&mVertexMemory
			) == VK_SUCCESS, "Failed to create vertex staging buffer for mesh"
		);

		if (indexBufferSize > 0)
		{
			COSMOS_ASSERT
			(
				mRenderer->GetDevice()->CreateBuffer
				(
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					indexBufferSize,
					&mIndexBuffer,
					&mIndexMemory
				) == VK_SUCCESS, "Failed to create index staging buffer for mesh"
			);
		}

		// copy from staging buffer into device local ones
		auto& renderpass = mRenderer->GetRenderpassManager()->GetMainRenderpass()->GetSpecificationRef();
		VkCommandBuffer copyCmd = mRenderer->GetDevice()->CreateCommandBuffer(renderpass.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferCopy copyRegion = {};
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(copyCmd, vertexStagingBuffer, mVertexBuffer, 1, &copyRegion);

		if (indexBufferSize > 0)
		{
			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(copyCmd, vertexStagingBuffer, mIndexBuffer, 1, &copyRegion);
		}

		mRenderer->GetDevice()->EndCommandBuffer(renderpass.commandPool, copyCmd, mRenderer->GetDevice()->GetGraphicsQueue(), true);

		// cleanup
		vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), vertexStagingBuffer, vertexStagingMemory);

		if (indexBufferSize > 0)
		{
			vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), IndexStagingBuffer, IndexStagingMemory);
		}

		delete[] loaderInfo.vertexbuffer;
		delete[] loaderInfo.indexbuffer;
	}
} 

#endif