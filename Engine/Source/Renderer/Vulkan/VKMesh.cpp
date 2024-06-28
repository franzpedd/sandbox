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

namespace Cosmos::Vulkan
{
	VKMesh::Node::~Node()
	{
		for (auto& child : children)
		{
			delete child;
		}
	}
	
	VKMesh::VKMesh(Shared<VKRenderer> renderer)
		: mRenderer(renderer)
	{
		mMaterial.colormapPath = GetAssetSubDir("Texture/Default/Mesh_Colormap.png");
	}

	VKMesh::~VKMesh()
	{
		vkDeviceWaitIdle(mRenderer->GetDevice()->GetLogicalDevice());
		
		// camera ubo
		for (size_t i = 0; i < mRenderer->GetConcurrentlyRenderedFramesCount(); i++)
		{
			vmaUnmapMemory(mRenderer->GetDevice()->GetAllocator(), mUniformBuffersMemory[i]);
			vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), mUniformBuffers[i], mUniformBuffersMemory[i]);
		}

		// util ubo
		for (size_t i = 0; i < mRenderer->GetConcurrentlyRenderedFramesCount(); i++)
		{
			vmaUnmapMemory(mRenderer->GetDevice()->GetAllocator(), mUtilitiesBuffersMemory[i]);
			vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), mUtilitiesBuffers[i], mUtilitiesBuffersMemory[i]);
		}
		
		vkDestroyDescriptorPool(mRenderer->GetDevice()->GetLogicalDevice(), mDescriptorPool, nullptr);
		vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), mVertexBuffer, mVertexMemory);
		
		if (mIndicesCount > 0)
		{
			vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), mIndexBuffer, mIndexMemory);
		}
		
		for (auto node : mNodes)
		{
			delete node;
		}
	}

	void VKMesh::OnUpdate(float timestep, glm::mat4& transform)
	{
		// update mvp
		if (!mLoaded) return;

		MVP_Buffer ubo = {};
		ubo.model = transform;
		ubo.view = mRenderer->GetCamera()->GetViewRef() * 2.0f;
		ubo.projection = mRenderer->GetCamera()->GetProjectionRef();
		ubo.cameraPos = mRenderer->GetCamera()->GetPositionRef();

		memcpy(mUniformBuffersMapped[mRenderer->GetCurrentFrame()], &ubo, sizeof(ubo));

		int x, y;
		Window::GetMousePosition(&x, &y);
		auto extent = mRenderer->GetSwapchain()->GetExtent();

		// update utils
		Util_Buffer util_ubo = {};
		util_ubo.selected = (mPicked == true ? (float)1.0f : (float)0.0f);
		util_ubo.picking = mRenderer->GetPicking();
		util_ubo.mousePos = glm::vec2((float)x, (float)y);
		util_ubo.windowSize = glm::vec2(extent.width, extent.height);
		memcpy(mUtilitiesBuffersMapped[mRenderer->GetCurrentFrame()], &util_ubo, sizeof(util_ubo));
	}

	void VKMesh::OnRender(void* commandBuffer)
	{
		uint32_t currentFrame = mRenderer->GetCurrentFrame();
		VkDeviceSize offsets[] = { 0 };
		VkCommandBuffer cmdBuffer = (VkCommandBuffer)commandBuffer; //mRenderer->GetRenderpassManager()->GetMainRenderpass()->GetSpecificationRef().commandBuffers[currentFrame];
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;

		if (mWiredframe)
		{
			pipeline = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Wireframed"]->GetPipeline();
			pipelineLayout = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Wireframed"]->GetPipelineLayout();
		}

		else
		{
			pipeline = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Common"]->GetPipeline();
			pipelineLayout = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Common"]->GetPipelineLayout();
		}

		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &mVertexBuffer, offsets);
		vkCmdBindIndexBuffer(cmdBuffer, mIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &mDescriptorSets[mRenderer->GetCurrentFrame()], 0, NULL);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

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

		const tinygltf::Scene& scene = model.scenes[0];

		for (size_t i = 0; i < scene.nodes.size(); i++)
		{
			const tinygltf::Node node = model.nodes[scene.nodes[i]];
			LoadGLTFNode(node, model, nullptr, mRawIndexBuffer, mRawVertexBuffer);
		}

		size_t vertexBufferSize = mRawVertexBuffer.size() * sizeof(Vertex);
		size_t indexBufferSize = mRawIndexBuffer.size() * sizeof(uint32_t);
		mIndicesCount = static_cast<uint32_t>(mRawIndexBuffer.size());

		CreateRendererResources(vertexBufferSize, indexBufferSize);
		SetupDescriptors();
		UpdateDescriptors();

		mFilepath = filepath;
		mLoaded = true;
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

	void VKMesh::LoadGLTFNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer)
	{
		Node* node = new Node();
		node->matrix = glm::mat4(1.0f);
		node->parent = parent;

		// get the local node matrix, it's either made up from translation, rotation, scale or a 4x4 matrix
		if (inputNode.translation.size() == 3)
		{
			node->matrix = glm::translate(node->matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
		}

		if (inputNode.rotation.size() == 4)
		{
			glm::quat q = glm::make_quat(inputNode.rotation.data());
			node->matrix *= glm::mat4(q);
		}

		if (inputNode.scale.size() == 3)
		{
			node->matrix = glm::scale(node->matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
		}

		if (inputNode.matrix.size() == 16)
		{
			node->matrix = glm::make_mat4x4(inputNode.matrix.data());
		}

		// load node's children
		if (inputNode.children.size() > 0)
		{
			for (size_t i = 0; i < inputNode.children.size(); i++)
			{
				LoadGLTFNode(input.nodes[inputNode.children[i]], input, node, indexBuffer, vertexBuffer);
			}
		}

		// if the node contains mesh data, we load vertices and indices from the buffers, in glTF this is done via accessors and buffer views
		if (inputNode.mesh > -1)
		{
			const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];

			// iterate through all primitives of this node's mesh
			for (size_t i = 0; i < mesh.primitives.size(); i++)
			{
				const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
				uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
				uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
				uint32_t indexCount = 0;

				// vertices
				{
					const float* positionBuffer = nullptr;
					const float* normalsBuffer = nullptr;
					const float* texCoordsBuffer = nullptr;
					size_t vertexCount = 0;

					// get buffer data for vertex positions
					if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
					{
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						vertexCount = accessor.count;
					}

					// get buffer data for vertex normals
					if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
					{
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}

					// get buffer data for vertex texture coordinates, GLTF supports multiple sets, we only load the first one
					if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
					{
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}

					// append data to mesh's vertex buffer
					for (size_t v = 0; v < vertexCount; v++)
					{
						Vertex vert{};
						vert.position = glm::vec3(glm::make_vec3(&positionBuffer[v * 3]));
						vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
						vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
						vert.color = glm::vec4(1.0f);
						vertexBuffer.push_back(vert);
					}
				}

				// indices
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
					const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

					indexCount += static_cast<uint32_t>(accessor.count);

					// glTF supports different component types of indices
					switch (accessor.componentType)
					{
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
						{
							const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
							for (size_t index = 0; index < accessor.count; index++)
							{
								indexBuffer.push_back(buf[index] + vertexStart);
							}

							break;
						}

						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
						{
							const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
							for (size_t index = 0; index < accessor.count; index++)
							{
								indexBuffer.push_back(buf[index] + vertexStart);
							}

							break;
						}

						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
						{
							const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
							for (size_t index = 0; index < accessor.count; index++)
							{
								indexBuffer.push_back(buf[index] + vertexStart);
							}

							break;
						}
						default:
						{
							COSMOS_LOG(Logger::Error, "Component type %d is not supported", accessor.componentType);
							return;
						}
					}
				}

				Primitive primitive = {};
				primitive.firstIndex = firstIndex;
				primitive.indexCount = indexCount;
				node->mesh.primitives.push_back(primitive);
			}
		}

		if (parent)
		{
			parent->children.push_back(node);
		}

		else
		{
			mNodes.push_back(node);
		}
	}

	void VKMesh::DrawNode(Node* node, VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
	{
		if (node->mesh.primitives.size() > 0)
		{
			for (Primitive& primitive : node->mesh.primitives)
			{
				if (primitive.indexCount > 0)
				{
					vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
				}
			}
		}

		for (auto& child : node->children)
		{
			DrawNode(child, commandBuffer, pipelineLayout);
		}
	}

	void VKMesh::CreateRendererResources(size_t vertexBufferSize, size_t indexBufferSize)
	{
		VkBuffer vertexStagingBuffer = VK_NULL_HANDLE;
		VmaAllocation vertexStagingMemory = VK_NULL_HANDLE;
		VkBuffer indexStagingBuffer = VK_NULL_HANDLE;
		VmaAllocation indexStagingMemory = VK_NULL_HANDLE;

		// create host visible staging buffers (source)
		COSMOS_ASSERT(mRenderer->GetDevice()->CreateBuffer(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBufferSize,
			&vertexStagingBuffer,
			&vertexStagingMemory,
			mRawVertexBuffer.data()
		) == VK_SUCCESS, "Failed to create Vertex Staging Buffer");

		if (mIndicesCount > 0)
		{
			COSMOS_ASSERT(mRenderer->GetDevice()->CreateBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				indexBufferSize,
				&indexStagingBuffer,
				&indexStagingMemory,
				mRawIndexBuffer.data()
			) == VK_SUCCESS, "Failed to create Index Staging Buffer");
		}
		
		// create device local buffers
		COSMOS_ASSERT(mRenderer->GetDevice()->CreateBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBufferSize,
			&mVertexBuffer,
			&mVertexMemory
		) == VK_SUCCESS, "Failed to create Vertex Staging Buffer");

		if (mIndicesCount > 0)
		{
			COSMOS_ASSERT(mRenderer->GetDevice()->CreateBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				indexBufferSize,
				&mIndexBuffer,
				&mIndexMemory
			) == VK_SUCCESS, "Failed to create Vertex Staging Buffer");
		}
		
		// copy from staging buffer to device local buffer
		auto& renderpass = mRenderer->GetRenderpassManager()->GetMainRenderpass()->GetSpecificationRef();
		VkCommandBuffer cmdbuffer = mRenderer->GetDevice()->CreateCommandBuffer(renderpass.commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkBufferCopy copyRegion = {};
		copyRegion.size = vertexBufferSize;
		vkCmdCopyBuffer(cmdbuffer, vertexStagingBuffer, mVertexBuffer, 1, &copyRegion);
		
		if (mIndicesCount > 0)
		{
			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(cmdbuffer, indexStagingBuffer, mIndexBuffer, 1, &copyRegion);
		}

		mRenderer->GetDevice()->EndCommandBuffer(renderpass.commandPool, cmdbuffer, mRenderer->GetDevice()->GetGraphicsQueue(), true);

		// free staging resources
		vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), vertexStagingBuffer, vertexStagingMemory);
		vmaDestroyBuffer(mRenderer->GetDevice()->GetAllocator(), indexStagingBuffer, indexStagingMemory);
	}

	void VKMesh::SetupDescriptors()
	{
		// model * view * projection ubo
		mUniformBuffers.resize(mRenderer->GetConcurrentlyRenderedFramesCount());
		mUniformBuffersMemory.resize(mRenderer->GetConcurrentlyRenderedFramesCount());
		mUniformBuffersMapped.resize(mRenderer->GetConcurrentlyRenderedFramesCount());

		// utilities ubo
		mUtilitiesBuffers.resize(mRenderer->GetConcurrentlyRenderedFramesCount());
		mUtilitiesBuffersMemory.resize(mRenderer->GetConcurrentlyRenderedFramesCount());
		mUtilitiesBuffersMapped.resize(mRenderer->GetConcurrentlyRenderedFramesCount());

		for (size_t i = 0; i < mRenderer->GetConcurrentlyRenderedFramesCount(); i++)
		{
			// camera's ubo
			mRenderer->GetDevice()->CreateBuffer
			(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(MVP_Buffer),
				&mUniformBuffers[i],
				&mUniformBuffersMemory[i]
			);

			vmaMapMemory(mRenderer->GetDevice()->GetAllocator(), mUniformBuffersMemory[i], &mUniformBuffersMapped[i]);

			mRenderer->GetDevice()->CreateBuffer
			(
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(Util_Buffer),
				&mUtilitiesBuffers[i],
				&mUtilitiesBuffersMemory[i]
			);

			vmaMapMemory(mRenderer->GetDevice()->GetAllocator(), mUtilitiesBuffersMemory[i], &mUtilitiesBuffersMapped[i]);
		}

		// textures
		mMaterial.colormapTex = VKTexture2D::Create(mRenderer, mMaterial.colormapPath.c_str(), true);

		// descriptor pool and descriptor sets
		std::array<VkDescriptorPoolSize, 3> poolSizes = {};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = mRenderer->GetConcurrentlyRenderedFramesCount();
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[1].descriptorCount = mRenderer->GetConcurrentlyRenderedFramesCount();
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = mRenderer->GetConcurrentlyRenderedFramesCount();

		VkDescriptorPoolCreateInfo descPoolCI = {};
		descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descPoolCI.poolSizeCount = (uint32_t)poolSizes.size();
		descPoolCI.pPoolSizes = poolSizes.data();
		descPoolCI.maxSets = mRenderer->GetConcurrentlyRenderedFramesCount();
		COSMOS_ASSERT(vkCreateDescriptorPool(mRenderer->GetDevice()->GetLogicalDevice(), &descPoolCI, nullptr, &mDescriptorPool) == VK_SUCCESS, "Failed to create descriptor pool");

		VkDescriptorSetLayout descriptorSetLayout;

		if (mWiredframe)
		{
			descriptorSetLayout = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Common"]->GetDescriptorSetLayout();
		}

		else
		{
			descriptorSetLayout = mRenderer->GetPipelineLibrary()->GetPipelinesRef()["Mesh.Wireframed"]->GetDescriptorSetLayout();
		}

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
				cameraUBOInfo.buffer = mUniformBuffers[i];
				cameraUBOInfo.offset = 0;
				cameraUBOInfo.range = sizeof(MVP_Buffer);

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
			
			// utilities ubo
			{
				VkDescriptorBufferInfo utilitiesUBO = {};
				utilitiesUBO.buffer = mUtilitiesBuffers[i];
				utilitiesUBO.offset = 0;
				utilitiesUBO.range = sizeof(Util_Buffer);

				VkWriteDescriptorSet utilitiesUBODesc = {};
				utilitiesUBODesc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				utilitiesUBODesc.dstSet = mDescriptorSets[i];
				utilitiesUBODesc.dstBinding = 1;
				utilitiesUBODesc.dstArrayElement = 0;
				utilitiesUBODesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				utilitiesUBODesc.descriptorCount = 1;
				utilitiesUBODesc.pBufferInfo = &utilitiesUBO;
				descriptorWrites.push_back(utilitiesUBODesc);
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
				colorMapDec.dstBinding = 2;
				colorMapDec.dstArrayElement = 0;
				colorMapDec.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				colorMapDec.descriptorCount = 1;
				colorMapDec.pImageInfo = &colorMapInfo;
				descriptorWrites.push_back(colorMapDec);
			}

			vkUpdateDescriptorSets(mRenderer->GetDevice()->GetLogicalDevice(), (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}
}

#endif