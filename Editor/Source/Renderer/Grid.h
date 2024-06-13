#pragma once

#define COSMOS_RENDERER_VULKAN
#include <Engine.h>

namespace Cosmos
{
	class Grid : public Widget
	{
	public:

		// constructor
		Grid(Shared<Renderer> renderer);

		// destructor
		~Grid();

	public:

		// draws the entity (leave empty if not required)
		virtual void OnRender() override;

		// updates the entity (leave empty if doesnt required)
		virtual void OnUpdate() override;

	public:

		// toogles on/off if grid should be drawn
		void ToogleOnOff();

		// create all renderer resources
		void CreateRendererResources();

	private:

		Shared<Renderer> mRenderer;
		bool mVisible = true;

		Shared<Vulkan::Shader> mVertexShader;
		Shared<Vulkan::Shader> mFragmentShader;

		VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> mDescriptorSets;

		std::vector<VkBuffer> mUniformBuffers;
		std::vector<VmaAllocation> mUniformBuffersMemory;
		std::vector<void*> mUniformBuffersMapped;
	};
}