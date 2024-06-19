#include "epch.h"
#include "Mesh.h"

#include "Vulkan/VKMesh.h"
#include "Vulkan/VKRenderer.h"

namespace Cosmos
{
	Mesh::Primitive::Primitive(Material& material, uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount)
		: material(material), firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount)
	{
		hasIndices = indexCount > 0;
	}

	void Mesh::Primitive::SetBoundingBox(glm::vec3 min, glm::vec3 max)
	{
		boundingBox.GetMaxRef() = max;
		boundingBox.GetMinRef() = min;
		boundingBox.SetValid(true);
	}

	Shared<Mesh> Mesh::Create(Shared<Renderer> renderer)
	{
#if defined COSMOS_RENDERER_VULKAN
		return CreateShared<Vulkan::VKMesh>(std::dynamic_pointer_cast<Vulkan::VKRenderer>(renderer));
#else
		return Shared<Renderer>();
#endif
	}
}