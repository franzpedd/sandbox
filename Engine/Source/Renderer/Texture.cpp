#include "epch.h"
#include "Texture.h"

#include "Vulkan/VKRenderer.h"
#include "Vulkan/VKTexture.h"

#include "Util/Logger.h"

namespace Cosmos
{
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