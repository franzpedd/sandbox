#pragma once

#include <Engine.h>

namespace Cosmos
{
	class Viewport : public Widget
	{
	public:

		// constructor
		Viewport(Shared<Window> window, Shared<Renderer> renderer);

		// destructor
		virtual ~Viewport();

	public:

		// for user interface drawing
		virtual void OnUpdate();

		// for renderer drawing
		virtual void OnRender();

		// // called when the window is resized
		virtual void OnEvent(Shared<Event> event);

	private:

		// creates all renderer resources
		void CreateRendererResources();

		// creates all framebuffer resources
		void CreateFramebufferResources();

	private:

		Shared<Window> mWindow;
		Shared<Renderer> mRenderer;

		// ui resources
		ImVec2 mCurrentSize;
		ImVec2 mContentRegionMin;
		ImVec2 mContentRegionMax;
		ImVec2 mViewportPosition;

		// vulkan resources
		VkFormat mSurfaceFormat = VK_FORMAT_UNDEFINED;
		VkFormat mDepthFormat = VK_FORMAT_UNDEFINED;
		VkSampler mSampler = VK_NULL_HANDLE;

		VkImage mDepthImage = VK_NULL_HANDLE;
		VmaAllocation mDepthMemory = VK_NULL_HANDLE;
		VkImageView mDepthView = VK_NULL_HANDLE;

		std::vector<VkImage> mImages;
		std::vector<VmaAllocation> mImageMemories;
		std::vector<VkImageView> mImageViews;

		std::vector<VkDescriptorSet> mDescriptorSets;
	};
}