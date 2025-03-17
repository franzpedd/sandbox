#include "Renderer.h"

#include "Instance.h"
#include "Device.h"

namespace Cosmos::Vulkan
{
	Renderer::Renderer()
	{
	}

	void Vulkan::Renderer::Initialize()
	{
		mInstance = std::make_shared<Instance>("Cosmos", "Application", true);
		mDevice = std::make_shared<Device>(mInstance);
	}
}