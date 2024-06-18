#include "epch.h"
#include "Texture.h"

#include "Vulkan/VKRenderer.h"
#include "Vulkan/VKTexture.h"

#include "Util/Logger.h"

namespace Cosmos
{
	TextureSampler::AddressMode TextureSampler::WrapMode(int32_t wrap)
	{
		switch (wrap)
		{
			case -1:
			case 10497: return AddressMode::ADDRESS_MODE_REPEAT;
			case 33071: return AddressMode::ADDRESS_MODE_CLAMP_TO_EDGE;
			case 33648: return AddressMode::ADDRESS_MODE_MIRRORED_REPEAT;
		}

		COSMOS_LOG(Logger::Error, "Unknown wrap mode %d", wrap);
		return AddressMode::ADDRESS_MODE_REPEAT;
	}

	TextureSampler::Filter TextureSampler::FilterMode(int32_t filter)
	{
		switch (filter)
		{
			case -1:
			case 9728: return Filter::FILTER_NEAREST;
			case 9729: return Filter::FILTER_LINEAR;
			case 9984: return Filter::FILTER_NEAREST;
			case 9985: return Filter::FILTER_NEAREST;
			case 9986: return Filter::FILTER_LINEAR;
			case 9987: return Filter::FILTER_LINEAR;
		}

		COSMOS_LOG(Logger::Error, "Unknown filter mode %d", filter);
		return Filter::FILTER_NEAREST;
	}

	Shared<Texture2D> Cosmos::Texture2D::Create(Shared<Renderer> renderer, const char* path)
	{
#if defined COSMOS_RENDERER_VULKAN
		return CreateShared<Vulkan::VKTexture2D>(std::dynamic_pointer_cast<Vulkan::VKRenderer>(renderer), path);
#else
		return Shared<Texture2D>();
#endif
	}

	Shared<TextureCubemap> TextureCubemap::Create(Shared<Renderer> renderer, std::array<const char*, 6> paths)
	{
#if defined COSMOS_RENDERER_VULKAN
		return CreateShared<Vulkan::VKTextureCubemap>(std::dynamic_pointer_cast<Vulkan::VKRenderer>(renderer), paths);
#else
		return Shared<TextureCubemap>();
#endif
	}
}