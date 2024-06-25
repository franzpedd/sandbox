#include "epch.h"
#include "Mesh.h"

#include "Texture.h"
#include "Vulkan/VKMesh.h"
#include "Vulkan/VKRenderer.h"

namespace Cosmos
{
	Shared<Mesh> Mesh::Create(Shared<Renderer> renderer)
	{
#if defined COSMOS_RENDERER_VULKAN
		return CreateShared<Vulkan::VKMesh>(std::dynamic_pointer_cast<Vulkan::VKRenderer>(renderer));
#else
		return Shared<Renderer>();
#endif
	}
}