#pragma once

#include "Util/Memory.h"
#include <array>

namespace Cosmos
{
	// forward declarations
	class Renderer;

	class Texture2D
	{
	public:

		// creates a texture from an input file
		static Shared<Texture2D> Create(Shared<Renderer> renderer, const char* path);

		// constructor
		Texture2D() = default;

		// destructor
		virtual ~Texture2D() = default;

		// returns the texture width
		inline int32_t GetWidth() const { return mWidth; }

		// returns the texture height
		inline int32_t GetHeight() const { return mHeight; }

		// returns the mip levels
		inline int32_t GetMipLevels() const { return mMipLevels; }

	public: // vulkan objects

		// returns a reference to the image view
		virtual inline void* GetView() = 0;

		// returns a reference to the image sampler
		virtual inline void* GetSampler() = 0;

	protected:

		int32_t mWidth = 0;
		int32_t mHeight = 0;
		int32_t mMipLevels = 1;
	};

	class TextureCubemap
	{
	public:

		// creates a texture from an input file
		static Shared<TextureCubemap> Create(Shared<Renderer> renderer, std::array<const char*, 6> paths);

		// constructor
		TextureCubemap() = default;

		// destructor
		virtual ~TextureCubemap() = default;

		// returns the texture width
		inline int32_t GetWidth() const { return mWidth; }

		// returns the texture height
		inline int32_t GetHeight() const { return mHeight; }

		// returns the mip levels
		inline int32_t GetMipLevels() const { return mMipLevels; }

	public: // vulkan objects

		// returns a reference to the image view
		virtual inline void* GetView() = 0;

		// returns a reference to the image sampler
		virtual inline void* GetSampler() = 0;

	protected:

		int32_t mWidth = 0;
		int32_t mHeight = 0;
		int32_t mMipLevels = 1;
	};
}