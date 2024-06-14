#include "epch.h"
#include "Texture.h"

#include "Vulkan/VKRenderer.h"
#include "Vulkan/VKTexture.h"

#include "Util/Logger.h"

namespace Cosmos
{
	Shared<Texture2D> Cosmos::Texture2D::Create(Shared<Renderer> renderer, const char* path)
	{
		return CreateShared<Vulkan::VKTexture2D>(std::dynamic_pointer_cast<Vulkan::VKRenderer>(renderer), path);
	}

	Shared<TextureCubemap> TextureCubemap::Create(Shared<Renderer> renderer, std::array<const char*, 6> paths)
	{
		return CreateShared<Vulkan::VKTextureCubemap>(std::dynamic_pointer_cast<Vulkan::VKRenderer>(renderer), paths);
	}
}