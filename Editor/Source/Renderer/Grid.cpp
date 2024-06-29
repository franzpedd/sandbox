#include "Grid.h"

#include "UI/Viewport.h"

#include <chrono>

namespace Cosmos
{
	Grid::Grid(std::shared_ptr<Renderer> renderer)
		: Widget("Grid"), mRenderer(renderer)
	{
		CreateRendererResources();
	}

	Grid::~Grid()
	{
		auto& device = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer)->GetDevice();
		vkDeviceWaitIdle(device->GetLogicalDevice());

		vkDestroyDescriptorPool(device->GetLogicalDevice(), mDescriptorPool, nullptr);
	}

	void Grid::OnRender()
	{
		if (!mVisible) return;

		auto& vkRenderer = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer);
		uint32_t currentFrame = mRenderer->GetCurrentFrame();
		VkDeviceSize offsets[] = { 0 };
		VkCommandBuffer cmdBuffer = vkRenderer->GetRenderpassManager()->GetMainRenderpass()->GetSpecificationRef().commandBuffers[currentFrame];

		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkRenderer->GetPipelineLibrary()->GetPipelinesRef()["Grid"]->GetPipeline());
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkRenderer->GetPipelineLibrary()->GetPipelinesRef()["Grid"]->GetPipelineLayout(), 0, 1, &mDescriptorSets[currentFrame], 0, nullptr);
		vkCmdDraw(cmdBuffer, 6, 1, 0, 0);
	}

	void Grid::OnUpdate()
	{

	}

	void Grid::ToogleOnOff()
	{
		mVisible == true ? mVisible = false : mVisible = true;
	}

	void Grid::CreateRendererResources()
	{
		auto& vkRenderer = std::dynamic_pointer_cast<Vulkan::VKRenderer>(mRenderer);

		// pipeline
		{
			Vulkan::Pipeline::Specification gridSpecificaiton = {};
			gridSpecificaiton.renderPass = vkRenderer->GetRenderpassManager()->GetMainRenderpass();
			gridSpecificaiton.vertexShader = CreateShared<Vulkan::Shader>(vkRenderer->GetDevice(), Vulkan::Shader::Type::Vertex, "Grid.vert", GetAssetSubDir("Shader/grid.vert"));
			gridSpecificaiton.fragmentShader = CreateShared<Vulkan::Shader>(vkRenderer->GetDevice(), Vulkan::Shader::Type::Fragment, "Grid.frag", GetAssetSubDir("Shader/grid.frag"));
			gridSpecificaiton.vertexComponents = { };
			gridSpecificaiton.notPassingVertexData = true;
			gridSpecificaiton.bindings.resize(1);

			// camera ubo
			gridSpecificaiton.bindings[0].binding = 0;
			gridSpecificaiton.bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			gridSpecificaiton.bindings[0].descriptorCount = 1;
			gridSpecificaiton.bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			gridSpecificaiton.bindings[0].pImmutableSamplers = nullptr;

			// create
			vkRenderer->GetPipelineLibrary()->GetPipelinesRef()["Grid"] = CreateShared<Vulkan::Pipeline>(vkRenderer->GetDevice(), gridSpecificaiton);	

			// build the pipeline
			vkRenderer->GetPipelineLibrary()->GetPipelinesRef()["Grid"]->Build(vkRenderer->GetPipelineLibrary()->GetPipelineCache());
		}

		// create descriptor pool and descriptor sets
		{
			VkDescriptorPoolSize poolSize = {};
			poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSize.descriptorCount = (uint32_t)mRenderer->GetConcurrentlyRenderedFramesCount();

			VkDescriptorPoolCreateInfo descPoolCI = {};
			descPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descPoolCI.poolSizeCount = 1;
			descPoolCI.pPoolSizes = &poolSize;
			descPoolCI.maxSets = (uint32_t)mRenderer->GetConcurrentlyRenderedFramesCount();
			COSMOS_ASSERT(vkCreateDescriptorPool(vkRenderer->GetDevice()->GetLogicalDevice(), &descPoolCI, nullptr, &mDescriptorPool) == VK_SUCCESS, "Failed to create descriptor pool");

			std::vector<VkDescriptorSetLayout> layouts(mRenderer->GetConcurrentlyRenderedFramesCount(), vkRenderer->GetPipelineLibrary()->GetPipelinesRef()["Grid"]->GetDescriptorSetLayout());

			VkDescriptorSetAllocateInfo descSetAllocInfo = {};
			descSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descSetAllocInfo.descriptorPool = mDescriptorPool;
			descSetAllocInfo.descriptorSetCount = (uint32_t)mRenderer->GetConcurrentlyRenderedFramesCount();
			descSetAllocInfo.pSetLayouts = layouts.data();

			mDescriptorSets.resize(mRenderer->GetConcurrentlyRenderedFramesCount());
			COSMOS_ASSERT(vkAllocateDescriptorSets(vkRenderer->GetDevice()->GetLogicalDevice(), &descSetAllocInfo, mDescriptorSets.data()) == VK_SUCCESS, "Failed to allocate descriptor sets");

			for (size_t i = 0; i < mRenderer->GetConcurrentlyRenderedFramesCount(); i++)
			{
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = vkRenderer->GetCameraDataRef().buffers[i];
				bufferInfo.offset = 0;
				bufferInfo.range = sizeof(CameraBuffer);

				VkWriteDescriptorSet descriptorWrite{};
				descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrite.dstSet = mDescriptorSets[i];
				descriptorWrite.dstBinding = 0;
				descriptorWrite.dstArrayElement = 0;
				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrite.descriptorCount = 1;
				descriptorWrite.pBufferInfo = &bufferInfo;

				vkUpdateDescriptorSets(vkRenderer->GetDevice()->GetLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
			}
		}
	}
}