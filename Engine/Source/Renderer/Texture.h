#pragma once

#include "Util/Memory.h"
#include <array>

namespace Cosmos
{
	// forward declarations
	class Renderer;

	class TextureSampler
	{
	public:
		typedef enum class Filter
		{
			FILTER_NEAREST = 0,
			FILTER_LINEAR = 1
		} Filter;

		typedef enum class AddressMode
		{
			ADDRESS_MODE_REPEAT = 0,
			ADDRESS_MODE_MIRRORED_REPEAT = 1,
			ADDRESS_MODE_CLAMP_TO_EDGE = 2,
			ADDRESS_MODE_CLAMP_TO_BORDER = 3
		} AddressMode;

	public:

		// translates the wrap mode for the renderer api
		static AddressMode WrapMode(int32_t wrap);

		// translates the filter mode for the renderer api
		static Filter FilterMode(int32_t filter);

	public:

		Filter mag = Filter::FILTER_NEAREST;
		Filter min = Filter::FILTER_NEAREST;
		AddressMode u = AddressMode::ADDRESS_MODE_REPEAT;
		AddressMode v = AddressMode::ADDRESS_MODE_REPEAT;
		AddressMode w = AddressMode::ADDRESS_MODE_REPEAT;
	};

	class Texture2D
	{
	public:

		// creates a texture from an input file
		static Shared<Texture2D> Create(Shared<Renderer> renderer, const char* path, bool flip = false);

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
		static Shared<TextureCubemap> Create(Shared<Renderer> renderer, std::array<const char*, 6> paths, bool flip = false);

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