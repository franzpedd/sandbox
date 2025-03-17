#include "epch.h"
#if defined COSMOS_RENDERER_VULKAN

#include "Device.h"
#include "Pipeline.h"
#include "Renderpass.h"
#include "VKMesh.h"
#include "VKTexture.h"
#include "VKRenderer.h"
#include "Swapchain.h"
#include "Entity/Unique/Camera.h"
#include "Platform/Window.h"
#include "Renderer/Buffer.h"
#include "Util/Files.h"
#include "Util/Logger.h"

namespace Cosmos::Vulkan::GLTF
{
	Primitive::Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Cosmos::Mesh::Material& material)
		: firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material)
	{
		hasIndices = indexCount > 0;
	}

	void Primitive::SetBoundingBox(glm::vec3 min, glm::vec3 max)
	{
		bb.SetMin(min);
		bb.SetMax(max);
		bb.SetValid(true);
	}

	Mesh::Mesh(Shared<Cosmos::Vulkan::Device> device, glm::mat4 matrix)
		: device(device)
	{
		uniformBlock.matrix = matrix;

		COSMOS_ASSERT
		(
			device->CreateBuffer
			(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(uniformBlock),
				&uniformBuffer.buffer,
				&uniformBuffer.memory,
				&uniformBlock
			) == VK_SUCCESS, "Failed to create Unfiform Buffer"
		);

		COSMOS_ASSERT(vmaMapMemory(device->GetAllocator(), uniformBuffer.memory, &uniformBuffer.mapped) == VK_SUCCESS, "Failed to map memory");
		uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(uniformBlock) };
	}

	Mesh::~Mesh()
	{
		vkDeviceWaitIdle(device->GetLogicalDevice());
		vmaUnmapMemory(device->GetAllocator(), uniformBuffer.memory);
		vmaDestroyBuffer(device->GetAllocator(), uniformBuffer.buffer, uniformBuffer.memory);
		for (Primitive* p : primitives) delete p;
	}

	void Mesh::SetBoundingBox(glm::vec3 min, glm::vec3 max)
	{
		bb.SetMin(min);
		bb.SetMax(max);
		bb.SetValid(true);
	}

	Node::~Node()
	{
		if (mesh) 
			delete mesh;

		for (auto& child : children) 
			delete child;
	}

	glm::mat4 Node::GetLocalMatrix()
	{
		return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
	}

	glm::mat4 Node::GetMatrix()
	{
		glm::mat4 m = GetLocalMatrix();
		Node* p = parent;
		while (p)
		{
			m = p->GetLocalMatrix() * m;
			p = p->parent;
		}
		
		return m;
	}

	void Node::OnUpdate()
	{
		if (mesh)
		{
			glm::mat4 m = GetMatrix();
			if (skin)
			{
				mesh->uniformBlock.matrix = m;

				// update join matrices
				glm::mat4 inverseTransform = glm::inverse(m);
				size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);

				for (size_t i = 0; i < numJoints; i++)
				{
					Node* jointNode = skin->joints[i];
					glm::mat4 jointMat = jointNode->GetMatrix() * skin->inverseBindMatrices[i];
					jointMat = inverseTransform * jointMat;
					mesh->uniformBlock.jointMatrix[i] = jointMat;
				}

				mesh->uniformBlock.jointcount = static_cast<uint32_t>(numJoints);
				memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
			}

			else
			{
				memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
			}
		}

		for (auto& child : children)
		{
			child->OnUpdate();
		}
	}

	glm::vec4 Animation::Sampler::CubicSplineInterpolation(size_t index, float time, uint32_t stride)
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
		for (uint32_t i = 0; i < stride; i++)
		{
			float p0 = outputs[current + i + V];			// starting point at t = 0
			float m0 = delta * outputs[current + i + A];	// scaled starting tangent at t = 0
			float p1 = outputs[next + i + V];				// ending point at t = 1
			float m1 = delta * outputs[next + i + B];		// scaled ending tangent at t = 1
			pt[i] = ((2.f * t3 - 3.f * t2 + 1.f) * p0) + ((t3 - 2.f * t2 + t) * m0) + ((-2.f * t3 + 3.f * t2) * p1) + ((t3 - t2) * m0);
		}
		return pt;
	}

	void Animation::Sampler::Translate(size_t index, float time, Node* node)
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

	void Animation::Sampler::Scale(size_t index, float time, Node* node)
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

	void Animation::Sampler::Rotate(size_t index, float time, Node* node)
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
}

namespace Cosmos::Vulkan
{
	VKMesh::VKMesh(Shared<VKRenderer> renderer)
		: mRenderer(renderer)
	{
	}

	VKMesh::~VKMesh()
	{
		vkDeviceWaitIdle(mRenderer->GetDevice()->GetLogicalDevice());

		vkDestroyDescriptorPool(mRenderer->GetDevice()->GetLogicalDevice(), mDescriptorPool, nullptr);

		if (mVertexBuffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), mVertexBuffer, mVertexMemory);
			mVertexBuffer = VK_NULL_HANDLE;
		}

		if (mIndexBuffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), mIndexBuffer, mIndexMemory);
			mIndexBuffer = VK_NULL_HANDLE;
		}

		mAnimations.resize(0);

		for (auto node : mNodes) delete node;
		mNodes.resize(0);
		mLinearNodes.resize(0);

		for (auto skin : mSkins) delete skin;
		mSkins.resize(0);
	}

	void VKMesh::OnUpdate(float timestep)
	{
	}

	void VKMesh::OnRender(void* commandBuffer, glm::mat4& transform, uint32_t id)
	{
		uint32_t currentFrame = mRenderer->GetCurrentFrame();
		VkDeviceSize offsets[] = { 0 };

		//mRenderer->GetRenderpassManager()->GetMainRenderpass()->GetSpecificationRef().commandBuffers[currentFrame];
		VkCommandBuffer cmdBuffer = (VkCommandBuffer)commandBuffer; 
		VkPipelineLayout pipelineLayout = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Common"]->GetPipelineLayout();
		VkPipeline pipeline = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Common"]->GetPipeline();

		if (mWiredframe)
		{
			pipeline = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Wireframed"]->GetPipeline();
			pipelineLayout = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Wireframed"]->GetPipelineLayout();
		}

		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mVertexBuffer, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, mIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &mDescriptorSets[mRenderer->GetCurrentFrame()], 0, NULL);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		ObjectPushConstant constants = {};
		constants.id = id;
		constants.model = transform;
		vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ObjectPushConstant), &constants);

		// render all nodes at top-level
		for (auto& node : mNodes)
		{
			DrawNode(node, cmdBuffer, pipelineLayout);
		}
	}

	void VKMesh::LoadFromFile(std::string filepath, float scale)
	{
		tinygltf::Model model;
		tinygltf::TinyGLTF context;
		std::string error, warning;

		bool fileLoaded = context.LoadASCIIFromFile(&model, &error, &warning, filepath);

		if (!fileLoaded)
		{
			COSMOS_LOG(Logger::Error, "Failed to load mesh %s, error: %s", filepath.c_str(), error.c_str());
			return;
		}

		if (warning.size() > 0)
		{
			COSMOS_LOG(Logger::Error, "Loading mesh %s with warning(s): %s", filepath.c_str(), warning.c_str());
		}

		size_t vertexCount = 0;
		size_t indexCount = 0;

		LoadMaterials(model);

		const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		// get vertex and index buffer sizes up-front
		for (size_t i = 0; i < scene.nodes.size(); i++)
		{
			GetNodeProperties(model.nodes[scene.nodes[i]], model, vertexCount, indexCount);
		}

		LoaderInfo loaderInfo = {};
		loaderInfo.vertexBuffer = new Vertex[vertexCount];
		loaderInfo.indexBuffer = new uint32_t[indexCount];

		mVertices.resize(vertexCount);

		COSMOS_LOG(Logger::Todo, "Handle scene with no default scene");

		for (size_t i = 0; i < scene.nodes.size(); i++)
		{
			const tinygltf::Node node = model.nodes[scene.nodes[i]];
			LoadNode(nullptr, node, scene.nodes[i], model, loaderInfo, scale);
		}

		if (model.animations.size() > 0)
		{
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

			// initial pose
			if (node->mesh)
			{
				node->OnUpdate();
			}
		}

		CreateRendererResources(vertexCount, indexCount, loaderInfo);
		SetupDescriptors();
		UpdateDescriptors();

		CalculateMeshDimension();

		mFilepath = filepath;
		mLoaded = true;

		delete[] loaderInfo.vertexBuffer;
		delete[] loaderInfo.indexBuffer;
	}

	Mesh::Dimension VKMesh::GetDimension() const
	{
		return mDimension;
	}

	void VKMesh::SetColormapTexture(std::string filepath)
	{
		vkDeviceWaitIdle(mRenderer->GetDevice()->GetLogicalDevice());
		
		if (mMaterial.colormapTex)
		{
			mMaterial.colormapTex.reset();
			mMaterial.colormapPath = filepath;
			mMaterial.colormapTex = Texture2D::Create(mRenderer, mMaterial.colormapPath.c_str(), true);
		}
		
		UpdateDescriptors();
	}

	Shared<Texture2D> VKMesh::GetColormapTexture()
	{
		return mMaterial.colormapTex;
	}

	void VKMesh::UpdateAnimation(uint32_t index, float time)
	{
		if (mAnimations.empty())
		{
			COSMOS_LOG(Logger::Error, "Mesh does not contain animation");
			return;
		}

		if (index > (uint32_t)(mAnimations.size()) - 1)
		{
			COSMOS_LOG(Logger::Error, "Mesh does not contain animation with index %d", index);
			return;
		}

		GLTF::Animation& animation = mAnimations[index];

		bool updated = false;

		for (auto& channel : animation.channels)
		{
			GLTF::Animation::Sampler& sampler = animation.samplers[channel.samplerIndex];

			if (sampler.inputs.size() > sampler.outputsVec4.size())
			{
				continue;
			}

			for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
			{
				if ((time >= sampler.inputs[i]) && (time <= sampler.inputs[i + 1]))
				{
					float u = std::max(0.0f, time - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);

					if (u <= 1.0f)
					{
						switch (channel.path)
						{
							case GLTF::Animation::Channel::PathType::TRANSLATION:
							{
								sampler.Translate(i, time, channel.node);
								break;
							}
								
							case GLTF::Animation::Channel::PathType::SCALE:
							{
								sampler.Scale(i, time, channel.node);
								break;
							}
								
							case GLTF::Animation::Channel::PathType::ROTATION:
							{
								sampler.Rotate(i, time, channel.node);
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

	void VKMesh::DrawNode(GLTF::Node* node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
	{
		if (node->mesh->primitives.size() > 0)
		{
			for (GLTF::Primitive* primitive : node->mesh->primitives)
			{
				if (primitive->indexCount > 0)
				{
					vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
				}
			}
		}

		for (auto& child : node->children)
		{
			DrawNode(child, commandBuffer, pipelineLayout);
		}
	}

	void VKMesh::LoadNode(GLTF::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, LoaderInfo& loaderInfo, float globalscale)
	{
		GLTF::Node* newNode = new GLTF::Node();
		newNode->index = nodeIndex;
		newNode->parent = parent;
		newNode->name = node.name;
		newNode->skinIndex = node.skin;
		newNode->matrix = glm::mat4(1.0f);

		// generate local node matrix
		glm::vec3 translation = glm::vec3(0.0f);

		if (node.translation.size() == 3)
		{
			translation = glm::make_vec3(node.translation.data());
			newNode->translation = translation;
		}

		glm::mat4 rotation = glm::mat4(1.0f);
		if (node.rotation.size() == 4)
		{
			glm::quat q = glm::make_quat(node.rotation.data());
			newNode->rotation = glm::mat4(q);
		}

		glm::vec3 scale = glm::vec3(1.0f);
		if (node.scale.size() == 3)
		{
			scale = glm::make_vec3(node.scale.data());
			newNode->scale = scale;
		}

		if (node.matrix.size() == 16)
		{
			newNode->matrix = glm::make_mat4x4(node.matrix.data());
		}

		// node with children
		if (node.children.size() > 0)
		{
			for (size_t i = 0; i < node.children.size(); i++) 
			{
				LoadNode(newNode, model.nodes[node.children[i]], node.children[i], model, loaderInfo, globalscale);
			}
		}

		// node contains mesh data
		if (node.mesh > -1)
		{
			const tinygltf::Mesh mesh = model.meshes[node.mesh];
			GLTF::Mesh* newMesh = new GLTF::Mesh(mRenderer->GetDevice(), newNode->matrix);

			for (size_t j = 0; j < mesh.primitives.size(); j++)
			{
				const tinygltf::Primitive& primitive = mesh.primitives[j];
				uint32_t vertexStart = static_cast<uint32_t>(loaderInfo.vertexPos);
				uint32_t indexStart = static_cast<uint32_t>(loaderInfo.indexPos);
				uint32_t indexCount = 0;
				uint32_t vertexCount = 0;
				glm::vec3 posMin{};
				glm::vec3 posMax{};
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
					int color0ByteStride;
					int jointByteStride;
					int weightByteStride;

					int jointComponentType;

					// Position attribute is required
					assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

					const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
					bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
					posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
					posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
					vertexCount = static_cast<uint32_t>(posAccessor.count);
					posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

					if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
					{
						const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
						const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
						bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
						normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
					}

					// uv0
					if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
					{
						const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
						const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
						bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
						uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
					}

					// vertex colors
					if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
					{
						const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
						const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
						bufferColorSet0 = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						color0ByteStride = accessor.ByteStride(view) ? (accessor.ByteStride(view) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
					}

					// skinning joints
					if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
					{
						const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
						const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
						bufferJoints = &(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
						jointComponentType = jointAccessor.componentType;
						jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointComponentType)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
					}

					// skinning weights
					if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
					{
						const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
						const tinygltf::BufferView& weightView = model.bufferViews[weightAccessor.bufferView];
						bufferWeights = reinterpret_cast<const float*>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
						weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
					}

					hasSkin = (bufferJoints && bufferWeights);

					for (size_t v = 0; v < posAccessor.count; v++)
					{
						Vertex& vert = loaderInfo.vertexBuffer[loaderInfo.vertexPos];
						vert.position = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
						vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
						vert.uv = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
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
									COSMOS_LOG(Logger::Error, "Joint component type %d is not supported", jointComponentType);
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
						if (glm::length(vert.weight) == 0.0f)
						{
							vert.weight = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
						}

						loaderInfo.vertexPos++;
						mVertices[v] = vert;
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
							for (size_t index = 0; index < accessor.count; index++)
							{
								loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
								loaderInfo.indexPos++;
							}

							break;
						}

						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
						{
							const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++)
							{
								loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
								loaderInfo.indexPos++;
							}

							break;
						}

						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
						{
							const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++)
							{
								loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
								loaderInfo.indexPos++;
							}

							break;
						}

						default:
						{
							COSMOS_LOG(Logger::Error, "Inex component type %d is not supported");
							return;
						}
					}
				}

				GLTF::Primitive* newPrimitive = new GLTF::Primitive(indexStart, indexCount, vertexCount, mMaterial);
				newPrimitive->SetBoundingBox(posMin, posMax);
				newMesh->primitives.push_back(newPrimitive);
			}

			// mesh BB from BBs of primitives
			for (auto p : newMesh->primitives)
			{
				if (p->bb.IsValid() && !newMesh->bb.IsValid())
				{
					newMesh->bb = p->bb;
					newMesh->bb.SetValid(true);;
				}

				newMesh->bb.SetMin(glm::min(newMesh->bb.GetMin(), p->bb.GetMin()));
				newMesh->bb.SetMax(glm::max(newMesh->bb.GetMax(), p->bb.GetMax()));
			}

			newNode->mesh = newMesh;
		}

		if (parent)
		{
			parent->children.push_back(newNode);
		}

		else
		{
			mNodes.push_back(newNode);
		}

		mLinearNodes.push_back(newNode);
	}

	void VKMesh::GetNodeProperties(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount)
	{
		if (node.children.size() > 0)
		{
			for (size_t i = 0; i < node.children.size(); i++) {
				GetNodeProperties(model.nodes[node.children[i]], model, vertexCount, indexCount);
			}
		}

		if (node.mesh > -1)
		{
			const tinygltf::Mesh mesh = model.meshes[node.mesh];
			for (size_t i = 0; i < mesh.primitives.size(); i++)
			{
				auto primitive = mesh.primitives[i];
				vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;

				if (primitive.indices > -1)
				{
					indexCount += model.accessors[primitive.indices].count;
				}
			}
		}
	}

	void VKMesh::LoadMaterials(tinygltf::Model& model)
	{
		COSMOS_LOG(Logger::Todo, "Implement better support for materials, only supporting one material per mesh");

		mMaterial.index = (int32_t)0;
		mMaterial.name = "Material";
		mMaterial.colormapPath = GetAssetSubDir("Texture/Default/Mesh_Colormap.png");
		mMaterial.colormapTex = VKTexture2D::Create(mRenderer, mMaterial.colormapPath.c_str(), true);
	}

	void VKMesh::LoadAnimations(tinygltf::Model& gltfModel)
	{
		for (tinygltf::Animation& anim : gltfModel.animations)
		{
			GLTF::Animation animation = {};
			animation.name = anim.name;

			if (anim.name.empty())
			{
				animation.name = std::to_string(mAnimations.size());
			}

			// samplers
			for (auto& samp : anim.samplers)
			{
				GLTF::Animation::Sampler sampler = {};

				if (samp.interpolation == "LINEAR")
				{
					sampler.interpolation = GLTF::Animation::Sampler::InterpolationType::LINEAR;
				}

				if (samp.interpolation == "STEP")
				{
					sampler.interpolation = GLTF::Animation::Sampler::InterpolationType::STEP;
				}

				if (samp.interpolation == "CUBICSPLINE")
				{
					sampler.interpolation = GLTF::Animation::Sampler::InterpolationType::CUBICSPLINE;
				}

				// read sampler input time values
				{
					const tinygltf::Accessor& accessor = gltfModel.accessors[samp.input];
					const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

					assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

					const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
					const float* buf = static_cast<const float*>(dataPtr);

					for (size_t index = 0; index < accessor.count; index++)
					{
						sampler.inputs.push_back(buf[index]);
					}

					for (auto input : sampler.inputs)
					{
						if (input < animation.start)
						{
							animation.start = input;
						}

						if (input > animation.end)
						{
							animation.end = input;
						}
					}
				}

				// read sampler output T/R/S values 
				{
					const tinygltf::Accessor& accessor = gltfModel.accessors[samp.output];
					const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

					assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

					const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

					switch (accessor.type)
					{
						case TINYGLTF_TYPE_VEC3:
						{
							const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++)
							{
								sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
								sampler.outputs.push_back(buf[index][0]);
								sampler.outputs.push_back(buf[index][1]);
								sampler.outputs.push_back(buf[index][2]);
							}
							break;
						}

						case TINYGLTF_TYPE_VEC4:
						{
							const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
							for (size_t index = 0; index < accessor.count; index++)
							{
								sampler.outputsVec4.push_back(buf[index]);
								sampler.outputs.push_back(buf[index][0]);
								sampler.outputs.push_back(buf[index][1]);
								sampler.outputs.push_back(buf[index][2]);
								sampler.outputs.push_back(buf[index][3]);
							}
							break;
						}

						default:
						{
							COSMOS_LOG(Logger::Error, "Unknown type of animation sampler output");
							break;
						}
					}
				}

				animation.samplers.push_back(sampler);
			}

			// channels
			for (auto& source : anim.channels)
			{
				GLTF::Animation::Channel channel{};

				if (source.target_path == "rotation")
				{
					channel.path = GLTF::Animation::Channel::PathType::ROTATION;
				}

				if (source.target_path == "translation")
				{
					channel.path = GLTF::Animation::Channel::PathType::TRANSLATION;
				}

				if (source.target_path == "scale")
				{
					channel.path = GLTF::Animation::Channel::PathType::SCALE;
				}

				if (source.target_path == "weights")
				{
					COSMOS_LOG(Logger::Error, "Weights is not yet supported, skipping channel");
					continue;
				}

				channel.samplerIndex = source.sampler;
				channel.node = GetNodeFromIndex(source.target_node);

				if (!channel.node)
				{
					continue;
				}

				animation.channels.push_back(channel);
			}

			mAnimations.push_back(animation);
		}
	}

	void VKMesh::LoadSkins(tinygltf::Model& gltfModel)
	{
		for (tinygltf::Skin& source : gltfModel.skins)
		{
			GLTF::Skin* newSkin = new GLTF::Skin();
			newSkin->name = source.name;

			// find skeleton root node
			if (source.skeleton > -1)
			{
				newSkin->skeletonRoot = GetNodeFromIndex(source.skeleton);
			}

			// find joint nodes
			for (int jointIndex : source.joints)
			{
				GLTF::Node* node = GetNodeFromIndex(jointIndex);
				if (node)
				{
					newSkin->joints.push_back(GetNodeFromIndex(jointIndex));
				}
			}

			// get inverse bind matrices from buffer
			if (source.inverseBindMatrices > -1)
			{
				const tinygltf::Accessor& accessor = gltfModel.accessors[source.inverseBindMatrices];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
				newSkin->inverseBindMatrices.resize(accessor.count);
				memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
			}

			mSkins.push_back(newSkin);
		}
	}

	GLTF::Node* VKMesh::GetNode(GLTF::Node* parent, uint32_t index)
	{
		GLTF::Node* found = nullptr;

		if (parent->index == index)
		{
			return parent;
		}

		for (auto& child : parent->children)
		{
			found = GetNode(child, index);

			if (found)
			{
				break;
			}
		}
		return found;
	}

	GLTF::Node* VKMesh::GetNodeFromIndex(uint32_t index)
	{
		GLTF::Node* found = nullptr;
		for (auto& node : mNodes)
		{
			found = GetNode(node, index);

			if (found)
			{
				break;
			}
		}

		return found;
	}

	void VKMesh::CreateRendererResources(size_t vertexCount, size_t indexCount, LoaderInfo& loaderInfo)
	{
		size_t vertexBufferSize = vertexCount * sizeof(Vertex);
		size_t indexBufferSize = indexCount * sizeof(uint32_t);

		struct StagingBuffer
		{
			VkBuffer buffer;
			VmaAllocation memory;
		} vertexStaging, indexStaging;

		// create staging buffers
		COSMOS_ASSERT(mRenderer->GetDevice()->CreateBuffer
		(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBufferSize,
			&vertexStaging.buffer,
			&vertexStaging.memory,
			loaderInfo.vertexBuffer) == VK_SUCCESS, "Failed to create vertex staging buffer"
		);

		if (indexBufferSize > 0)
		{
			COSMOS_ASSERT(mRenderer->GetDevice()->CreateBuffer
			(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				indexBufferSize,
				&indexStaging.buffer,
				&indexStaging.memory,
				loaderInfo.indexBuffer) == VK_SUCCESS, "Failed to create index staging buffer"
			);
		}

		// create device local buffers
		COSMOS_ASSERT(mRenderer->GetDevice()->CreateBuffer
		(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBufferSize,
			&mVertexBuffer,
			&mVertexMemory) == VK_SUCCESS, "Failed to create vertex local buffer"
		);

		// index buffer
		if (indexBufferSize > 0)
		{
			COSMOS_ASSERT(mRenderer->GetDevice()->CreateBuffer
			(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				indexBufferSize,
				&mIndexBuffer,
				&mIndexMemory) == VK_SUCCESS, "Failed to create index local buffer"
			);
		}

		// copy from staging buffer to device local buffer
		auto& renderpass = mRenderer->GetRenderpassManager()->GetMainRenderpass()->GetSpecificationRef();
		VkCommandBuffer cmdbuffer = mRenderer->GetDevice()->CreateCommandBuffer(renderpass.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferCopy copyRegion = {};
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(cmdbuffer, vertexStaging.buffer, mVertexBuffer, 1, & copyRegion);

		if (indexBufferSize > 0)
		{
			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(cmdbuffer, indexStaging.buffer, mIndexBuffer, 1, &copyRegion);
		}

		mRenderer->GetDevice()->EndCommandBuffer(renderpass.commandPool, cmdbuffer, mRenderer->GetDevice()->GetGraphicsQueue(), true);

		// free staging resources
		vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), vertexStaging.buffer, vertexStaging.memory);

		if (indexBufferSize > 0)
		{
			vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), indexStaging.buffer, indexStaging.memory);
		}
	}

	void VKMesh::SetupDescriptors()
	{
		// descriptor pool and descriptor sets
		std::array<VkDescriptorPoolSize, 2> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = mRenderer->GetConcurrentlyRenderedFramesCount();
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = mRenderer->GetConcurrentlyRenderedFramesCount();

		VkDescriptorPoolCreateInfo descPoolCI = {};
		descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descPoolCI.poolSizeCount = (uint32_t)poolSizes.size();
		descPoolCI.pPoolSizes = poolSizes.data();
		descPoolCI.maxSets = mRenderer->GetConcurrentlyRenderedFramesCount();
		COSMOS_ASSERT(vkCreateDescriptorPool(mRenderer->GetDevice()->GetLogicalDevice(), &descPoolCI, nullptr, &mDescriptorPool) == VK_SUCCESS, "Failed to create descriptor pool");

		VkDescriptorSetLayout descriptorSetLayout = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Common"]->GetDescriptorSetLayout();

		std::vector<VkDescriptorSetLayout> layouts
		(
			mRenderer->GetConcurrentlyRenderedFramesCount(),
			descriptorSetLayout
		);

		VkDescriptorSetAllocateInfo descSetAllocInfo = {};
		descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descSetAllocInfo.descriptorPool = mDescriptorPool;
		descSetAllocInfo.descriptorSetCount = (uint32_t)mRenderer->GetConcurrentlyRenderedFramesCount();
		descSetAllocInfo.pSetLayouts = layouts.data();

		mDescriptorSets.resize(mRenderer->GetConcurrentlyRenderedFramesCount());
		COSMOS_ASSERT(vkAllocateDescriptorSets(mRenderer->GetDevice()->GetLogicalDevice(), &descSetAllocInfo, mDescriptorSets.data()) == VK_SUCCESS, "Failed to allocate descriptor sets");
	}

	void VKMesh::UpdateDescriptors()
	{
		for (size_t i = 0; i < mRenderer->GetConcurrentlyRenderedFramesCount(); i++)
		{
			std::vector<VkWriteDescriptorSet> descriptorWrites = {};

			// camera ubo
			{
				VkDescriptorBufferInfo cameraUBOInfo = {};
				cameraUBOInfo.buffer = mRenderer->GetCameraDataRef().buffers[i];
				cameraUBOInfo.offset = 0;
				cameraUBOInfo.range = sizeof(CameraBuffer);

				VkWriteDescriptorSet cameraUBODesc = {};
				cameraUBODesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				cameraUBODesc.dstSet = mDescriptorSets[i];
				cameraUBODesc.dstBinding = 0;
				cameraUBODesc.dstArrayElement = 0;
				cameraUBODesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				cameraUBODesc.descriptorCount = 1;
				cameraUBODesc.pBufferInfo = &cameraUBOInfo;
				descriptorWrites.push_back(cameraUBODesc);
			}

			// window ubo
			{
				VkDescriptorBufferInfo windowUBOInfo = {};
				windowUBOInfo.buffer = mRenderer->GetWindowDataRef().buffers[i];
				windowUBOInfo.offset = 0;
				windowUBOInfo.range = sizeof(WindowBuffer);

				VkWriteDescriptorSet windowUBODesc = {};
				windowUBODesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				windowUBODesc.dstSet = mDescriptorSets[i];
				windowUBODesc.dstBinding = 1;
				windowUBODesc.dstArrayElement = 0;
				windowUBODesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				windowUBODesc.descriptorCount = 1;
				windowUBODesc.pBufferInfo = &windowUBOInfo;
				descriptorWrites.push_back(windowUBODesc);
			}

			// storage ubo
			{
				VkDescriptorBufferInfo storageBufferInfo = {};
				storageBufferInfo.buffer = mRenderer->GetPickingDataRef().buffers[i];
				storageBufferInfo.offset = 0;
				storageBufferInfo.range = sizeof(PickingDepthBuffer);

				VkWriteDescriptorSet storageDesc = {};
				storageDesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				storageDesc.dstSet = mDescriptorSets[i];
				storageDesc.dstBinding = 2;
				storageDesc.dstArrayElement = 0;
				storageDesc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				storageDesc.descriptorCount = 1;
				storageDesc.pBufferInfo = &storageBufferInfo;
				descriptorWrites.push_back(storageDesc);
			}

			// color map
			{
				VkDescriptorImageInfo colorMapInfo = {};
				colorMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				colorMapInfo.imageView = (VkImageView)mMaterial.colormapTex->GetView();
				colorMapInfo.sampler = (VkSampler)mMaterial.colormapTex->GetSampler();

				VkWriteDescriptorSet colorMapDec = {};
				colorMapDec.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				colorMapDec.dstSet = mDescriptorSets[i];
				colorMapDec.dstBinding = 3;
				colorMapDec.dstArrayElement = 0;
				colorMapDec.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				colorMapDec.descriptorCount = 1;
				colorMapDec.pImageInfo = &colorMapInfo;
				descriptorWrites.push_back(colorMapDec);
			}

			vkUpdateDescriptorSets(mRenderer->GetDevice()->GetLogicalDevice(), (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

	void VKMesh::CalculateMeshDimension()
	{
		// calculate binary volume hierarchy for all nodes in the scene
		for (auto node : mLinearNodes)
		{
			CalculateBoundingBox(node, nullptr);
		}

		mDimension.min = glm::vec3(FLT_MAX);
		mDimension.max = glm::vec3(-FLT_MAX);

		for (auto node : mLinearNodes)
		{
			if (node->bvh.IsValid())
			{
				mDimension.min = glm::min(mDimension.min, node->bvh.GetMin());
				mDimension.max = glm::max(mDimension.max, node->bvh.GetMax());
			}
		}

		// calculate scene aabb
		mDimension.aabb = glm::scale(glm::mat4(1.0f), glm::vec3(mDimension.max[0] - mDimension.min[0], mDimension.max[1] - mDimension.min[1], mDimension.max[2] - mDimension.min[2]));
		mDimension.aabb[3][0] = mDimension.min[0];
		mDimension.aabb[3][1] = mDimension.min[1];
		mDimension.aabb[3][2] = mDimension.min[2];
	}

	void VKMesh::CalculateBoundingBox(GLTF::Node* node, GLTF::Node* parent)
	{
		Physics::BoundingBox parentBvh = parent ? parent->bvh : Physics::BoundingBox(mDimension.min, mDimension.max);

		if (node->mesh)
		{
			if (node->mesh->bb.IsValid())
			{
				node->aabb = node->mesh->bb.GetAABB(node->GetMatrix());

				if (node->children.size() == 0)
				{
					node->bvh.SetMin(node->aabb.GetMin());
					node->bvh.SetMax(node->aabb.GetMax());
					node->bvh.SetValid(true);
				}
			}
		}

		parentBvh.SetMin(glm::min(parentBvh.GetMin(), node->bvh.GetMin()));
		parentBvh.SetMax(glm::min(parentBvh.GetMax(), node->bvh.GetMax()));

		for (auto& child : node->children)
		{
			CalculateBoundingBox(child, node);
		}
	}
}

#endif