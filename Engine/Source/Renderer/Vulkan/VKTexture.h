#pragma once

#include "Renderer/Texture.h"
#include <volk.h>
#include "Wrapper/vma.h" // including vma after volk

namespace Cosmos::Vulkan
{
	// forward declarations
	class VKRenderer;

	class VKTexture2D : public Texture2D
	{
	public:

		// constructor
		VKTexture2D(Shared<VKRenderer> renderer, const char* path);

		// destructor
		~VKTexture2D();

	public:

		// returns a reference to the image view
		virtual inline void* GetView() override { return mView; }

		// returns a reference to the image sampler
		virtual inline void* GetSampler() override { return mSampler; }

	private:

		// loads the texture based on constructor's path
		void LoadTexture();

		// creates mipmaps for the current bound texture
		void CreateMipmaps();

	private:

		Shared<VKRenderer> mRenderer;
		const char* mPath = nullptr;

		VkImage mImage = VK_NULL_HANDLE;
		VmaAllocation mMemory = VK_NULL_HANDLE;
		VkImageView mView = VK_NULL_HANDLE;
		VkSampler mSampler = VK_NULL_HANDLE;
	};

	class VKTextureCubemap : public TextureCubemap
	{
	public:

		// constructor
		VKTextureCubemap(Shared<VKRenderer> renderer, std::array<const char*, 6> paths);

		// destructor
		~VKTextureCubemap();

	public:

		// returns a reference to the image view
		virtual inline void* GetView() override { return mView; }

		// returns a reference to the image sampler
		virtual inline void* GetSampler() { return mSampler; }

	private:

		// loads the texture based on constructor's path
		void LoadTextures();

	private:

		Shared<VKRenderer> mRenderer;
		std::array<const char*, 6> mPaths = {};

		VkImage mImage = VK_NULL_HANDLE;
		VmaAllocation mMemory = VK_NULL_HANDLE;
		VkImageView mView = VK_NULL_HANDLE;
		VkSampler mSampler = VK_NULL_HANDLE;
	};
}