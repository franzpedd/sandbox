#include "epch.h"
#if defined COSMOS_RENDERER_VULKAN

#include "VKTexture.h"

#include "VKRenderer.h"
#include "Device.h"
#include "Renderpass.h"

#include "Util/Logger.h"

#if defined(PLATFORM_WINDOWS)
	#pragma warning(push)
	#pragma warning(disable : 26827)
#endif

#include <stb_image.h>

#if defined(PLATFORM_WINDOWS)
	#pragma warning(pop)
#endif

namespace Cosmos::Vulkan
{
	VKTexture2D::VKTexture2D(Shared<VKRenderer> renderer, std::string path, bool flip)
		: mRenderer(renderer)
	{
		mPath = path;
		stbi_set_flip_vertically_on_load(flip);
		LoadTexture();

		// image view
		mView = mRenderer->GetDevice()->CreateImageView
		(
			mImage,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_ASPECT_COLOR_BIT,
			mMipLevels
		);

		// sampler
		mSampler = mRenderer->GetDevice()->CreateSampler
		(
			VK_FILTER_LINEAR,
			VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			(float)mMipLevels
		);
	}

	VKTexture2D::~VKTexture2D()
	{
		vkDeviceWaitIdle(mRenderer->GetDevice()->GetLogicalDevice());

		vkDestroyImageView(mRenderer->GetDevice()->GetLogicalDevice(), mView, nullptr);
		vkDestroyImage(mRenderer->GetDevice()->GetLogicalDevice(), mImage, nullptr);
		vmaFreeMemory(mRenderer->GetDevice()->GetAllocator(), mMemory);
		vkDestroySampler(mRenderer->GetDevice()->GetLogicalDevice(), mSampler, nullptr);
	}

	void VKTexture2D::LoadTexture()
	{
		int32_t channels;
		stbi_uc* pixels = stbi_load(mPath.c_str(), &mWidth, &mHeight, &channels, STBI_rgb_alpha);
		
		if (pixels == nullptr)
		{
			COSMOS_LOG(Logger::Assert, "Failed to load %s texture", mPath.c_str());
			return;
		}

		mMipLevels = (uint32_t)(std::floor(std::log2(std::max(mWidth, mHeight)))) + 1;
		VkDeviceSize imgSize = (VkDeviceSize)(mWidth * mHeight * 4); // enforce 4 channels

		// create staging buffer for image
		VkBuffer stagingBuffer;
		VmaAllocation stagingMemory;

		mRenderer->GetDevice()->CreateBuffer
		(
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			imgSize,
			&stagingBuffer,
			&stagingMemory
		);

		void* data = nullptr;
		vmaMapMemory(mRenderer->GetDevice()->GetAllocator(), stagingMemory, &data);
		memcpy(data, pixels, (size_t)imgSize);
		vmaUnmapMemory(mRenderer->GetDevice()->GetAllocator(), stagingMemory);

		stbi_image_free(pixels);

		auto& renderpass = mRenderer->GetRenderpassManager()->GetMainRenderpass()->GetSpecificationRef();

		// create image resource
		mRenderer->GetDevice()->CreateImage
		(
			mWidth,
			mHeight,
			mMipLevels,
			1,
			renderpass.msaa,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			mImage,
			mMemory
		);

		// transition layout to transfer data
		mRenderer->GetDevice()->TransitionImageLayout
		(
			renderpass.commandPool,
			mImage,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			mMipLevels
		);

		// copy buffer to image
		{
			VkCommandBuffer cmdBuffer = mRenderer->GetDevice()->BeginSingleTimeCommand(renderpass.commandPool);

			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;
			region.imageOffset.x = 0;
			region.imageOffset.y = 0;
			region.imageOffset.z = 0;
			region.imageExtent.width = mWidth;
			region.imageExtent.height = mHeight;
			region.imageExtent.depth = 1;
			vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			mRenderer->GetDevice()->EndSingleTimeCommand(renderpass.commandPool, cmdBuffer);
		}

		// free staging buffer
		vkDestroyBuffer(mRenderer->GetDevice()->GetLogicalDevice(), stagingBuffer, nullptr);
		vmaFreeMemory(mRenderer->GetDevice()->GetAllocator(), stagingMemory);

		CreateMipmaps();
	}

	void VKTexture2D::CreateMipmaps()
	{
		auto& renderpass = mRenderer->GetRenderpassManager()->GetMainRenderpass()->GetSpecificationRef();

		VkCommandBuffer commandBuffer = mRenderer->GetDevice()->BeginSingleTimeCommand(renderpass.commandPool);

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = mImage;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = mWidth;
		int32_t mipHeight = mHeight;

		for (int32_t i = 1; i < mMipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit = {};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mMipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		mRenderer->GetDevice()->EndSingleTimeCommand(renderpass.commandPool, commandBuffer);
	}

	VKTextureCubemap::VKTextureCubemap(Shared<VKRenderer> renderer, std::array<std::string, 6> paths, bool flip)
		: mRenderer(renderer)
	{
		mPaths = paths;
		COSMOS_LOG(Logger::Todo, "Create mipmaps for Vulkan Cubemaps");

		stbi_set_flip_vertically_on_load(flip);
		LoadTextures();

		// image view
		mView = mRenderer->GetDevice()->CreateImageView
		(
			mImage,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_ASPECT_COLOR_BIT,
			mMipLevels,
			6,
			VK_IMAGE_VIEW_TYPE_CUBE
		);

		// sampler
		mSampler = mRenderer->GetDevice()->CreateSampler
		(
			VK_FILTER_LINEAR,
			VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			VK_SAMPLER_ADDRESS_MODE_REPEAT,
			(float)mMipLevels
		);
	}

	VKTextureCubemap::~VKTextureCubemap()
	{
		vkDeviceWaitIdle(mRenderer->GetDevice()->GetLogicalDevice());

		vkDestroyImageView(mRenderer->GetDevice()->GetLogicalDevice(), mView, nullptr);
		vkDestroyImage(mRenderer->GetDevice()->GetLogicalDevice(), mImage, nullptr);
		vmaFreeMemory(mRenderer->GetDevice()->GetAllocator(), mMemory);
		vkDestroySampler(mRenderer->GetDevice()->GetLogicalDevice(), mSampler, nullptr);
	}

	void VKTextureCubemap::LoadTextures()
	{
		COSMOS_ASSERT(mPaths.size() == 6, "A cubemap must have 6 textures");

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferMemory = VK_NULL_HANDLE;

		VkDeviceSize layerSize;
		VkDeviceSize imageSize;
		int32_t channels;

		void* data;
		uint64_t memAddress;

		stbi_set_flip_vertically_on_load(true);

		for (uint8_t i = 0; i < mPaths.size(); i++)
		{
			stbi_uc* pixels = stbi_load(mPaths[i].c_str(), &mWidth, &mHeight, &channels, STBI_rgb_alpha);

			if (pixels == nullptr)
			{
				COSMOS_LOG(Logger::Assert, "Failed to load %s texture", mPaths[i].c_str());
				return;
			}

			if (i == 0)
			{
				// this 4 should be mChannels, however RGBA is widely supported on GPU as RGB-only is not
				layerSize = mWidth * mHeight * 4;
				imageSize = layerSize * mPaths.size();

				mRenderer->GetDevice()->CreateBuffer
				(
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					imageSize,
					&stagingBuffer,
					&stagingBufferMemory
				);

				vmaMapMemory(mRenderer->GetDevice()->GetAllocator(), stagingBufferMemory, &data);
				memAddress = reinterpret_cast<uint64_t>(data);
			}

			memcpy(reinterpret_cast<uint8_t*>(memAddress), static_cast<void*>(pixels), static_cast<size_t>(layerSize));
			stbi_image_free(pixels);
			memAddress += layerSize;

			if (i == 5)
			{
				vmaUnmapMemory(mRenderer->GetDevice()->GetAllocator(), stagingBufferMemory);
			}
		}

		auto& renderpass = mRenderer->GetRenderpassManager()->GetMainRenderpass()->GetSpecificationRef();

		// create image resource
		mRenderer->GetDevice()->CreateImage
		(
			mWidth,
			mHeight,
			mMipLevels,
			6,
			renderpass.msaa,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			mImage,
			mMemory,
			VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
		);

		// transition layout to transfer data
		mRenderer->GetDevice()->TransitionImageLayout
		(
			renderpass.commandPool,
			mImage,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			mMipLevels,
			(uint32_t)mPaths.size()
		);

		// copy buffer to image
		{
			VkCommandBuffer cmdBuffer = mRenderer->GetDevice()->BeginSingleTimeCommand(renderpass.commandPool);

			VkBufferImageCopy region = {};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 6;
			region.imageOffset.x = 0;
			region.imageOffset.y = 0;
			region.imageOffset.z = 0;
			region.imageExtent.width = mWidth;
			region.imageExtent.height = mHeight;
			region.imageExtent.depth = 1;
			vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer, mImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			mRenderer->GetDevice()->EndSingleTimeCommand(renderpass.commandPool, cmdBuffer);
		}

		// transition layout to shader read only
		mRenderer->GetDevice()->TransitionImageLayout
		(
			renderpass.commandPool,
			mImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			mMipLevels,
			(uint32_t)mPaths.size()
		);

		// free staging buffer
		vkDestroyBuffer(mRenderer->GetDevice()->GetLogicalDevice(), stagingBuffer, nullptr);
		vmaFreeMemory(mRenderer->GetDevice()->GetAllocator(), stagingBufferMemory);
	}
}

#endif